#include "file_manager.h"
#include "logger.h"
#include "mapper_dll.h"
#include "net_utils.h"
#include "phase4_config.h"
#include "worker_client.h"

#include <filesystem>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

#ifdef _WIN32
#define LIB_HANDLE HMODULE
#define LOAD_LIB(path) LoadLibraryA(path)
#define GET_SYM(lib, name) GetProcAddress(lib, name)
#define CLOSE_LIB(lib) FreeLibrary(lib)
#define LIB_EXT ".dll"
#else
#define LIB_HANDLE void*
#define LOAD_LIB(path) nullptr
#define GET_SYM(lib, name) nullptr
#define CLOSE_LIB(lib)
#define LIB_EXT ""
#endif

int main(int argc, char* argv[]) {
    if (argc != 9) {
        std::cerr << "Usage: mapper_worker.exe <dll_directory> <input_file> <temp_directory> <output_directory> <mapper_id> <reducer_count> <controller_host> <controller_port>" << std::endl;
        return 1;
    }

    WinsockInitializer winsock;
    if (!winsock.ok()) {
        std::cerr << "Winsock initialization failed." << std::endl;
        return 1;
    }

    const fs::path dll_directory = argv[1];
    const fs::path input_file = argv[2];
    const fs::path temp_directory = argv[3];
    const fs::path output_directory = argv[4];
    const int mapper_id = std::stoi(argv[5]);
    const int reducer_count = std::stoi(argv[6]);
    const std::string controller_host = argv[7];
    const int controller_port = std::stoi(argv[8]);

    Phase4Config config;
    FileManager file_manager;

    file_manager.ensure_directory(temp_directory);
    file_manager.ensure_directory(config.logs_directory(output_directory));
    Logger::instance().initialize(config.mapper_log_path(output_directory, mapper_id), false);
    Logger::instance().info("Mapper worker starting for file: " + input_file.string());

    WorkerClient client;
    if (!client.connect_and_register("MAP", mapper_id, controller_host, controller_port)) {
        Logger::instance().error("Mapper failed to register with controller.");
        return 1;
    }

    if (!client.wait_for_begin()) {
        Logger::instance().error("Mapper failed waiting for BEGIN.");
        return 1;
    }

    client.start_heartbeat(config.heartbeat_interval_seconds);

    if (!fs::exists(input_file)) {
        client.stop_heartbeat();
        client.send_error("Input_file_missing");
        Logger::instance().error("Input file does not exist: " + input_file.string());
        return 1;
    }

#ifdef _WIN32
    const fs::path mapper_dll_path = dll_directory / ("mapper_dll" LIB_EXT);
    LIB_HANDLE mapper_module = LOAD_LIB(mapper_dll_path.string().c_str());

    if (!mapper_module) {
        client.stop_heartbeat();
        client.send_error("Mapper_DLL_load_failed");
        Logger::instance().error("Failed to load mapper DLL: " + mapper_dll_path.string());
        return 1;
    }

    auto create_mapper = reinterpret_cast<decltype(&CreateMapper)>(GET_SYM(mapper_module, "CreateMapper"));
    auto destroy_mapper = reinterpret_cast<decltype(&DestroyMapper)>(GET_SYM(mapper_module, "DestroyMapper"));
    auto mapper_map = reinterpret_cast<decltype(&MapperMap)>(GET_SYM(mapper_module, "MapperMap"));
    auto mapper_flush = reinterpret_cast<decltype(&MapperFlush)>(GET_SYM(mapper_module, "MapperFlush"));

    if (!create_mapper || !destroy_mapper || !mapper_map || !mapper_flush) {
        client.stop_heartbeat();
        client.send_error("Mapper_DLL_missing_exports");
        Logger::instance().error("Mapper DLL is missing required exports.");
        CLOSE_LIB(mapper_module);
        return 1;
    }

    void* mapper = create_mapper(
        static_cast<void*>(&file_manager),
        temp_directory.string().c_str(),
        mapper_id,
        reducer_count,
        1000
    );

    if (!mapper) {
        client.stop_heartbeat();
        client.send_error("Mapper_instance_create_failed");
        Logger::instance().error("Failed to create mapper instance.");
        CLOSE_LIB(mapper_module);
        return 1;
    }

    try {
        const auto lines = file_manager.read_all_lines(input_file);

        for (const auto& line : lines) {
            if (!mapper_map(mapper, input_file.filename().string().c_str(), line.c_str())) {
                client.stop_heartbeat();
                client.send_error("Mapper_map_failed");
                Logger::instance().error("Mapper failed while processing input file.");
                destroy_mapper(mapper);
                CLOSE_LIB(mapper_module);
                return 1;
            }
        }
    }
    catch (const std::exception& ex) {
        client.stop_heartbeat();
        client.send_error("Mapper_read_exception");
        Logger::instance().error(std::string("Exception while reading mapper input file: ") + ex.what());
        destroy_mapper(mapper);
        CLOSE_LIB(mapper_module);
        return 1;
    }

    if (!mapper_flush(mapper)) {
        client.stop_heartbeat();
        client.send_error("Mapper_flush_failed");
        Logger::instance().error("Mapper flush failed.");
        destroy_mapper(mapper);
        CLOSE_LIB(mapper_module);
        return 1;
    }

    destroy_mapper(mapper);
    CLOSE_LIB(mapper_module);
    client.stop_heartbeat();
    client.send_complete();
    Logger::instance().info("Mapper worker completed successfully.");
    return 0;
#else
    client.stop_heartbeat();
    client.send_error("Mapper_only_supports_Windows");
    Logger::instance().fatal("Mapper worker currently supports Windows DLL loading only.");
    return 1;
#endif
}
