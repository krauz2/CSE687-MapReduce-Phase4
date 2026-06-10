#include "stub_server.h"

#include "logger.h"
#include "message_protocol.h"
#include "net_utils.h"

#include <map>
#include <string>
#include <vector>

#include <windows.h>

namespace {
    std::wstring widen(const std::string& text) {
        return std::wstring(text.begin(), text.end());
    }

    std::wstring quote_arg(const std::string& text) {
        return L"\"" + widen(text) + L"\"";
    }

    bool launch_process(const std::string& exe_path, const std::vector<std::string>& args) {
        std::wstring command_line = quote_arg(exe_path);
        for (const auto& arg : args) {
            command_line += L" " + quote_arg(arg);
        }

        STARTUPINFOW startup_info{};
        startup_info.cb = sizeof(startup_info);

        PROCESS_INFORMATION process_info{};
        std::wstring mutable_command = command_line;

        const BOOL ok = CreateProcessW(
            widen(exe_path).c_str(),
            mutable_command.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &startup_info,
            &process_info
        );

        if (ok != TRUE) {
            return false;
        }

        CloseHandle(process_info.hThread);
        CloseHandle(process_info.hProcess);
        return true;
    }
}

StubServer::StubServer(int port)
    : port_(port) {
}

bool StubServer::run() {
    SOCKET listen_socket = create_listen_socket(port_);
    if (listen_socket == INVALID_SOCKET) {
        Logger::instance().error("Stub failed to create listening socket.");
        return false;
    }

    Logger::instance().info("Stub listening on port " + std::to_string(port_));

    while (true) {
        SOCKET client = accept_client(listen_socket);
        if (client == INVALID_SOCKET) {
            Logger::instance().warning("Stub accept failed, continuing.");
            continue;
        }

        std::string line;
        if (!recv_line(client, line)) {
            Logger::instance().warning("Stub received empty or invalid request.");
            close_socket(client);
            continue;
        }

        Logger::instance().info("Stub received: " + line);
        const ParsedMessage message = parse_message(line);

        std::map<std::string, std::string> ack_fields;
        ack_fields["worker_id"] = get_field(message, "worker_id", "-1");
        ack_fields["status"] = "FAIL";
        ack_fields["reason"] = "Bad_request";

        bool launched = false;

        if (message.valid && message.command == "SPAWN" && (message.role == "MAP" || message.role == "REDUCE")) {
            const std::string worker_exe = get_field(message, "worker_exe");
            const std::string dll_dir = get_field(message, "dll_dir");
            const std::string temp_dir = get_field(message, "temp_dir");
            const std::string output_dir = get_field(message, "output_dir");
            const std::string worker_id = get_field(message, "worker_id");
            const std::string controller_host = get_field(message, "controller_host");
            const std::string controller_port = get_field(message, "controller_port");

            std::vector<std::string> args;

            if (message.role == "MAP") {
                args = {
                    dll_dir,
                    get_field(message, "input_file"),
                    temp_dir,
                    output_dir,
                    worker_id,
                    get_field(message, "reducer_count"),
                    controller_host,
                    controller_port
                };
            }
            else {
                args = {
                    dll_dir,
                    temp_dir,
                    output_dir,
                    worker_id,
                    get_field(message, "mapper_count"),
                    controller_host,
                    controller_port
                };
            }

            launched = launch_process(worker_exe, args);
            ack_fields["status"] = launched ? "OK" : "FAIL";
            ack_fields["reason"] = launched ? "Spawned" : "CreateProcess_failed";
        }

        const std::string ack = build_message("SPAWN_ACK", message.role, ack_fields);
        send_line(client, ack);
        close_socket(client);
    }

    close_socket(listen_socket);
    return true;
}
