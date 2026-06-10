#pragma once

#include "net_utils.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

class WorkerClient {
public:
    WorkerClient();
    ~WorkerClient();

    bool connect_and_register(
        const std::string& role,
        int worker_id,
        const std::string& host,
        int port);

    bool wait_for_begin();
    void start_heartbeat(int interval_seconds);
    void stop_heartbeat();

    bool send_complete();
    bool send_error(const std::string& error_message);

private:
    void heartbeat_loop(int interval_seconds);

    SOCKET socket_ = INVALID_SOCKET;
    std::string role_;
    int worker_id_ = -1;

    std::mutex send_mutex_;
    std::thread heartbeat_thread_;
    std::atomic<bool> heartbeat_running_{ false };
};
