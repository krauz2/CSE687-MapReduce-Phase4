#pragma once

#include "net_utils.h"

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class ControllerServer {
public:
    explicit ControllerServer(int port);
    ~ControllerServer();

    bool start();
    void stop();

    void reset_phase(const std::string& role, int expected_count);
    bool wait_for_all_registered();
    bool send_begin_to_all();
    bool wait_for_all_completed();

private:
    struct WorkerState {
        SOCKET socket = INVALID_SOCKET;
        bool registered = false;
        bool completed = false;
        bool ok = false;
        std::string role;
        std::string error;
    };

    void accept_loop();
    void client_loop(SOCKET client);

    int port_;
    SOCKET listen_socket_ = INVALID_SOCKET;
    std::atomic<bool> running_{ false };
    std::thread accept_thread_;
    std::vector<std::thread> client_threads_;
    std::mutex threads_mutex_;

    std::string current_role_;
    int expected_count_ = 0;
    std::map<int, WorkerState> workers_;
    std::mutex state_mutex_;
    std::condition_variable state_cv_;
};
