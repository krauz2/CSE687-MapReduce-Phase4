#include "worker_client.h"

#include "message_protocol.h"

#include <chrono>
#include <thread>

WorkerClient::WorkerClient() = default;

WorkerClient::~WorkerClient() {
    stop_heartbeat();
    close_socket(socket_);
    socket_ = INVALID_SOCKET;
}

bool WorkerClient::connect_and_register(
    const std::string& role,
    int worker_id,
    const std::string& host,
    int port) {

    role_ = role;
    worker_id_ = worker_id;
    socket_ = connect_to_host(host, port);

    if (socket_ == INVALID_SOCKET) {
        return false;
    }

    const std::string registration = build_message(
        "REGISTER",
        role_,
        { {"worker_id", std::to_string(worker_id_)} });

    return send_line(socket_, registration);
}

bool WorkerClient::wait_for_begin() {
    std::string line;
    while (recv_line(socket_, line)) {
        const ParsedMessage message = parse_message(line);
        if (!message.valid) {
            continue;
        }

        if (message.command == "BEGIN" &&
            message.role == role_ &&
            get_int_field(message, "worker_id", -1) == worker_id_) {
            return true;
        }
    }

    return false;
}

void WorkerClient::start_heartbeat(int interval_seconds) {
    heartbeat_running_ = true;
    heartbeat_thread_ = std::thread(&WorkerClient::heartbeat_loop, this, interval_seconds);
}

void WorkerClient::stop_heartbeat() {
    heartbeat_running_ = false;
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
}

bool WorkerClient::send_complete() {
    std::lock_guard<std::mutex> lock(send_mutex_);
    const std::string message = build_message(
        "COMPLETE",
        role_,
        { {"worker_id", std::to_string(worker_id_)},
          {"status", "OK"} });

    return send_line(socket_, message);
}

bool WorkerClient::send_error(const std::string& error_message) {
    std::lock_guard<std::mutex> lock(send_mutex_);
    const std::string message = build_message(
        "ERROR",
        role_,
        { {"worker_id", std::to_string(worker_id_)},
          {"message", error_message} });

    return send_line(socket_, message);
}

void WorkerClient::heartbeat_loop(int interval_seconds) {
    while (heartbeat_running_) {
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));

        if (!heartbeat_running_) {
            break;
        }

        std::lock_guard<std::mutex> lock(send_mutex_);
        const std::string message = build_message(
            "HEARTBEAT",
            role_,
            { {"worker_id", std::to_string(worker_id_)},
              {"status", "RUNNING"} });

        if (!send_line(socket_, message)) {
            break;
        }
    }
}
