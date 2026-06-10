#pragma once

#include <filesystem>

class Controller {
public:
    bool run(
        const std::filesystem::path& input_directory,
        const std::filesystem::path& dll_directory,
        const std::filesystem::path& output_directory,
        const std::filesystem::path& temp_directory,
        int reducer_count
    );
};
