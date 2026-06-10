#include "stub_server.h"

#include "logger.h"
#include "net_utils.h"
#include "phase4_config.h"

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    WinsockInitializer winsock;
    if (!winsock.ok()) {
        std::cerr << "Winsock initialization failed." << std::endl;
        return 1;
    }

    Phase4Config config;
    const int port = (argc >= 2) ? std::stoi(argv[1]) : config.stub_endpoints.front().port;

    const std::filesystem::path log_path =
        std::filesystem::current_path() / ("stub_server_" + std::to_string(port) + ".log");

    Logger::instance().initialize(log_path, false);
    Logger::instance().info("Stub server starting on port " + std::to_string(port));

    StubServer server(port);
    const bool success = server.run();
    return success ? 0 : 1;
}
