#include "executive.h"

#include "controller.h"
#include "phase4_config.h"

#include <filesystem>
#include <iostream>

int Executive::run(int argc, char* argv[]) {
    Phase4Config config;

    if (argc < 3 || argc > 6) {
        std::cerr
            << "Usage:\n"
            << "  Phase4.exe <input_directory> <dll_directory> [output_directory] [temp_directory] [reducer_count]\n";
        return 1;
    }

    const std::filesystem::path input_directory = argv[1];
    const std::filesystem::path dll_directory = argv[2];

    const std::filesystem::path output_directory =
        (argc >= 4) ? std::filesystem::path(argv[3]) : std::filesystem::path("output");

    const std::filesystem::path temp_directory =
        (argc >= 5) ? std::filesystem::path(argv[4]) : std::filesystem::path("temp");

    const int reducer_count =
        (argc >= 6) ? std::stoi(argv[5]) : config.default_reducer_count;

    if (reducer_count <= 0) {
        std::cerr << "Reducer count must be greater than 0." << std::endl;
        return 1;
    }

    Controller controller;
    const bool success = controller.run(
        input_directory,
        dll_directory,
        output_directory,
        temp_directory,
        reducer_count
    );

    return success ? 0 : 1;
}
