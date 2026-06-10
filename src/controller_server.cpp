#include "controller_server.h"

#include "logger.h"
#include "message_protocol.h"

ControllerServer::ControllerServer(int port)
    : port_(port) {
}

ControllerServer::~ControllerServer() {
    stop();
}

bool ControllerServer::start() {
    listen_socket_ = create_listen_socket(port_);
    if (listen_socket_ == INVALID_SOCKET) {
        Logger::instance().error("Controller server failed to create listening socket.");
        return false;
    }

    running_ = true;
    accept_thread_ = std::thread(&ControllerServer::accept_loop, this);
    Logger::instance().info("Controller server listening on port " + std::to_string(port_));
    return true;
}

void ControllerServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    close_socket(listen_socket_);
    listen_socket_ = INVALID_SOCKET;

    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(threads_mutex_);
        for (auto& thread : client_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        client_threads_.clear();
    }

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        for (auto& [id, worker] : workers_) {
            close_socket(worker.socket);
            worker.socket = INVALID_SOCKET;
        }
        workers_.clear();
    }
}

void ControllerServer::reset_phase(const std::string& role, int expected_count) {
    std::lock_guard<std::mutex> lock(state_mutex_);

    for (auto& [id, worker] : workers_) {
        close_socket(worker.socket);
    }

    workers_.clear();
    current_role_ = role;
    expected_count_ = expected_count;

    Logger::instance().info("Controller server reset for phase role=" + role +
                            ", expected_count=" + std::to_string(expected_count));
}

bool ControllerServer::wait_for_all_registered() {
    std::unique_lock<std::mutex> lock(state_mutex_);
    state_cv_.wait(lock, [&] {
        int count = 0;
        for (const auto& [id, worker] : workers_) {
            if (worker.registered && worker.role == current_role_) {
                ++count;
            }
        }
        return count >= expected_count_;
    });

    Logger::instance().info("All " + current_role_ + " workers registered.");
    return true;
}

bool ControllerServer::send_begin_to_all() {
    std::vector<std::pair<int, SOCKET>> targets;

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        for (const auto& [id, worker] : workers_) {
            if (worker.registered && worker.role == current_role_) {
                targets.push_back({ id, worker.socket });
            }
        }
    }

    for (const auto& [id, socket_handle] : targets) {
        const std::string message = build_message(
            "BEGIN",
            current_role_,
            { {"worker_id", std::to_string(id)} });

        if (!send_line(socket_handle, message)) {
            Logger::instance().error("Failed to send BEGIN to worker " + std::to_string(id));
            return false;
        }
    }

    Logger::instance().info("BEGIN sent to all " + current_role_ + " workers.");
    return true;
}

bool ControllerServer::wait_for_all_completed() {
    std::unique_lock<std::mutex> lock(state_mutex_);
    state_cv_.wait(lock, [&] {
        int count = 0;
        for (const auto& [id, worker] : workers_) {
            if (worker.completed && worker.role == current_role_) {
                ++count;
            }
        }
        return count >= expected_count_;
    });

    for (const auto& [id, worker] : workers_) {
        if (worker.role == current_role_ && !worker.ok) {
            Logger::instance().error(
                "Worker " + std::to_string(id) + " failed. Reason: " + worker.error);
            return false;
        }
    }

    Logger::instance().info("All " + current_role_ + " workers completed successfully.");
    return true;
}

void ControllerServer::accept_loop() {
    while (running_) {
        SOCKET client = accept_client(listen_socket_);
        if (client == INVALID_SOCKET) {
            if (running_) {
                Logger::instance().warning("Controller accept failed.");
            }
            continue;
        }

        std::lock_guard<std::mutex> lock(threads_mutex_);
        client_threads_.emplace_back(&ControllerServer::client_loop, this, client);
    }
}

void ControllerServer::client_loop(SOCKET client) {
    int registered_worker_id = -1;

    while (true) {
        std::string line;
        if (!recv_line(client, line)) {
            break;
        }

        const ParsedMessage message = parse_message(line);
        if (!message.valid) {
            continue;
        }

        const int worker_id = get_int_field(message, "worker_id", -1);

        if (message.command == "REGISTER") {
            std::lock_guard<std::mutex> lock(state_mutex_);
            WorkerState& worker = workers_[worker_id];
            worker.socket = client;
            worker.registered = true;
            worker.role = message.role;
            worker.completed = false;
            worker.ok = false;
            worker.error.clear();
            registered_worker_id = worker_id;

            Logger::instance().info("Worker registered: role=" + message.role +
                                    ", id=" + std::to_string(worker_id));
            state_cv_.notify_all();
        }
        else if (message.command == "HEARTBEAT") {
            Logger::instance().info("Heartbeat received: role=" + message.role +
                                    ", id=" + std::to_string(worker_id));
        }
        else if (message.command == "COMPLETE") {
            std::lock_guard<std::mutex> lock(state_mutex_);
            WorkerState& worker = workers_[worker_id];
            worker.completed = true;
            worker.ok = (get_field(message, "status", "OK") == "OK");
            worker.role = message.role;
            registered_worker_id = worker_id;

            Logger::instance().info("Worker completed: role=" + message.role +
                                    ", id=" + std::to_string(worker_id));
            state_cv_.notify_all();
            break;
        }
        else if (message.command == "ERROR") {
            std::lock_guard<std::mutex> lock(state_mutex_);
            WorkerState& worker = workers_[worker_id];
            worker.completed = true;
            worker.ok = false;
            worker.role = message.role;
            worker.error = get_field(message, "message", "Unknown_error");
            registered_worker_id = worker_id;

            Logger::instance().error("Worker error: role=" + message.role +
                                     ", id=" + std::to_string(worker_id) +
                                     ", message=" + worker.error);
            state_cv_.notify_all();
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (registered_worker_id >= 0) {
            auto it = workers_.find(registered_worker_id);
            if (it != workers_.end()) {
                if (!it->second.completed) {
                    it->second.completed = true;
                    it->second.ok = false;
                    it->second.error = "socket_closed";
                    state_cv_.notify_all();
                }
                it->second.socket = INVALID_SOCKET;
            }
        }
    }

    close_socket(client);
}
