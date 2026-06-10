#include "controller.h"

#include "controller_server.h"
#include "file_manager.h"
#include "logger.h"
#include "message_protocol.h"
#include "net_utils.h"
#include "phase4_config.h"

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <windows.h>

namespace fs = std::filesystem;

namespace {
    fs::path get_executable_directory() {
        wchar_t buffer[MAX_PATH];
        const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (length == 0) {
            return fs::current_path();
        }
        return fs::path(buffer).parent_path();
    }

    bool send_spawn_request(const StubEndpoint& stub, const std::string& message) {
        SOCKET socket_handle = connect_to_host(stub.host, stub.port);
        if (socket_handle == INVALID_SOCKET) {
            return false;
        }

        if (!send_line(socket_handle, message)) {
            close_socket(socket_handle);
            return false;
        }

        std::string response;
        const bool ok = recv_line(socket_handle, response);
        close_socket(socket_handle);

        if (!ok) {
            return false;
        }

        const ParsedMessage parsed = parse_message(response);
        return parsed.valid &&
               parsed.command == "SPAWN_ACK" &&
               get_field(parsed, "status", "FAIL") == "OK";
    }
}

bool Controller::run(
    const fs::path& input_directory,
    const fs::path& dll_directory,
    const fs::path& output_directory,
    const fs::path& temp_directory,
    int reducer_count) {

    WinsockInitializer winsock;
    if (!winsock.ok()) {
        std::cerr << "Winsock initialization failed." << std::endl;
        return false;
    }

    FileManager file_manager;
    Phase4Config config;

    if (!file_manager.directory_exists(input_directory)) {
        std::cerr << "Input directory does not exist: " << input_directory << std::endl;
        return false;
    }

    if (!file_manager.directory_exists(dll_directory)) {
        std::cerr << "DLL directory does not exist: " << dll_directory << std::endl;
        return false;
    }

    if (!file_manager.ensure_directory(output_directory) ||
        !file_manager.ensure_directory(temp_directory)) {
        std::cerr << "Could not create/access output or temp directory." << std::endl;
        return false;
    }

    file_manager.clear_directory_contents(output_directory);
    file_manager.clear_directory_contents(temp_directory);
    file_manager.ensure_directory(output_directory);
    file_manager.ensure_directory(temp_directory);
    file_manager.ensure_directory(config.logs_directory(output_directory));

    Logger::instance().initialize(config.controller_log_path(output_directory), false);
    Logger::instance().info("Phase 4 controller starting.");

    const std::vector<fs::path> input_files = file_manager.get_input_files(input_directory);
    if (input_files.empty()) {
        Logger::instance().error("No input files found.");
        return false;
    }

    const int mapper_count = static_cast<int>(input_files.size());
    Logger::instance().info("Mapper process count: " + std::to_string(mapper_count));
    Logger::instance().info("Reducer process count: " + std::to_string(reducer_count));

    ControllerServer server(config.controller_port);
    if (!server.start()) {
        Logger::instance().error("Controller server failed to start.");
        return false;
    }

    const fs::path exe_dir = get_executable_directory();
    const std::string mapper_exe = (exe_dir / config.mapper_worker_exe_name).string();
    const std::string reducer_exe = (exe_dir / config.reducer_worker_exe_name).string();

    // Map phase.
    server.reset_phase("MAP", mapper_count);

    for (int mapper_id = 0; mapper_id < mapper_count; ++mapper_id) {
        const StubEndpoint& stub =
            config.stub_endpoints[static_cast<std::size_t>(mapper_id) % config.stub_endpoints.size()];

        const std::map<std::string, std::string> fields = {
            {"worker_id", std::to_string(mapper_id)},
            {"worker_exe", mapper_exe},
            {"dll_dir", dll_directory.string()},
            {"input_file", input_files[static_cast<std::size_t>(mapper_id)].string()},
            {"temp_dir", temp_directory.string()},
            {"output_dir", output_directory.string()},
            {"reducer_count", std::to_string(reducer_count)},
            {"controller_host", config.controller_host},
            {"controller_port", std::to_string(config.controller_port)}
        };

        const std::string request = build_message("SPAWN", "MAP", fields);
        if (!send_spawn_request(stub, request)) {
            Logger::instance().error("Failed to spawn mapper " + std::to_string(mapper_id));
            server.stop();
            return false;
        }
    }

    if (!server.wait_for_all_registered() ||
        !server.send_begin_to_all() ||
        !server.wait_for_all_completed()) {
        Logger::instance().error("Map phase failed.");
        server.stop();
        return false;
    }

    // Reduce phase.
    server.reset_phase("REDUCE", reducer_count);

    for (int reducer_id = 0; reducer_id < reducer_count; ++reducer_id) {
        const StubEndpoint& stub =
            config.stub_endpoints[static_cast<std::size_t>(reducer_id) % config.stub_endpoints.size()];

        const std::map<std::string, std::string> fields = {
            {"worker_id", std::to_string(reducer_id)},
            {"worker_exe", reducer_exe},
            {"dll_dir", dll_directory.string()},
            {"temp_dir", temp_directory.string()},
            {"output_dir", output_directory.string()},
            {"mapper_count", std::to_string(mapper_count)},
            {"controller_host", config.controller_host},
            {"controller_port", std::to_string(config.controller_port)}
        };

        const std::string request = build_message("SPAWN", "REDUCE", fields);
        if (!send_spawn_request(stub, request)) {
            Logger::instance().error("Failed to spawn reducer " + std::to_string(reducer_id));
            server.stop();
            return false;
        }
    }

    if (!server.wait_for_all_registered() ||
        !server.send_begin_to_all() ||
        !server.wait_for_all_completed()) {
        Logger::instance().error("Reduce phase failed.");
        server.stop();
        return false;
    }

    // Final merge.
    std::map<std::string, int> final_counts;

    for (int reducer_id = 0; reducer_id < reducer_count; ++reducer_id) {
        const fs::path reducer_output = config.reducer_output_path(output_directory, reducer_id);

        if (!fs::exists(reducer_output)) {
            continue;
        }

        std::ifstream input_file(reducer_output);
        std::string line;

        while (std::getline(input_file, line)) {
            const std::size_t tab_position = line.find('\t');
            if (tab_position == std::string::npos) {
                continue;
            }

            const std::string key = line.substr(0, tab_position);
            const int value = std::stoi(line.substr(tab_position + 1));
            final_counts[key] += value;
        }
    }

    std::vector<std::pair<std::string, int>> merged_records;
    merged_records.reserve(final_counts.size());
    for (const auto& entry : final_counts) {
        merged_records.push_back(entry);
    }

    if (!file_manager.reset_final_output_file(output_directory, config.final_output_file_name)) {
        Logger::instance().error("Failed to reset final output file.");
        server.stop();
        return false;
    }

    if (!file_manager.append_final_results(output_directory, config.final_output_file_name, merged_records)) {
        Logger::instance().error("Failed to write final merged output.");
        server.stop();
        return false;
    }

    if (!file_manager.create_success_file(output_directory)) {
        Logger::instance().error("Failed to create SUCCESS file.");
        server.stop();
        return false;
    }

    server.stop();
    Logger::instance().info("Phase 4 controller completed successfully.");
    return true;
}
