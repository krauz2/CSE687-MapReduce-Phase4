#include "file_manager.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

bool FileManager::directory_exists(const fs::path& directory) const {
    return fs::exists(directory) && fs::is_directory(directory);
}

bool FileManager::ensure_directory(const fs::path& directory) const {
    try {
        if (!fs::exists(directory)) {
            fs::create_directories(directory);
        }
        return fs::exists(directory) && fs::is_directory(directory);
    }
    catch (const std::exception& ex) {
        std::cerr << "Failed to create/access directory: " << directory
            << "\nReason: " << ex.what() << std::endl;
        return false;
    }
}

bool FileManager::clear_directory_contents(const fs::path& directory) const {
    try {
        if (!directory_exists(directory)) {
            return false;
        }

        for (const auto& entry : fs::directory_iterator(directory)) {
            fs::remove_all(entry.path());
        }

        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "Failed to clear directory: " << directory
            << "\nReason: " << ex.what() << std::endl;
        return false;
    }
}

bool FileManager::remove_file_if_exists(const fs::path& file_path) const {
    try {
        if (!fs::exists(file_path)) {
            return true;
        }
        return fs::remove(file_path);
    }
    catch (const std::exception& ex) {
        std::cerr << "Failed to remove file: " << file_path
            << "\nReason: " << ex.what() << std::endl;
        return false;
    }
}

bool FileManager::create_empty_file(const fs::path& file_path) const {
    try {
        if (!file_path.parent_path().empty()) {
            fs::create_directories(file_path.parent_path());
        }
        std::ofstream out(file_path, std::ios::trunc);
        return out.is_open();
    }
    catch (const std::exception& ex) {
        std::cerr << "Failed to create file: " << file_path
            << "\nReason: " << ex.what() << std::endl;
        return false;
    }
}

std::vector<fs::path> FileManager::get_input_files(const fs::path& input_directory) const {
    std::vector<fs::path> files;
    if (!directory_exists(input_directory)) {
        return files;
    }
    for (const auto& entry : fs::directory_iterator(input_directory)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::vector<std::string> FileManager::read_all_lines(const fs::path& file_path) const {
    std::vector<std::string> lines;
    std::ifstream input_file(file_path);
    if (!input_file.is_open()) {
        throw std::runtime_error("Could not open input file: " + file_path.string());
    }
    std::string line;
    while (std::getline(input_file, line)) {
        lines.push_back(line);
    }
    return lines;
}

bool FileManager::append_records_to_file(
    const fs::path& file_path,
    const std::vector<std::pair<std::string, int>>& records) const {
    try {
        if (!file_path.parent_path().empty()) {
            fs::create_directories(file_path.parent_path());
        }

        std::ofstream output_file(file_path, std::ios::app);
        if (!output_file.is_open()) {
            std::cerr << "Could not open file for append: " << file_path << std::endl;
            return false;
        }

        for (const auto& record : records) {
            output_file << record.first << '\t' << record.second << '\n';
        }

        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "Failed to append records. Reason: " << ex.what() << std::endl;
        return false;
    }
}

bool FileManager::append_final_results(
    const fs::path& output_directory,
    const std::string& output_file_name,
    const std::vector<std::pair<std::string, int>>& records) const {
    try {
        fs::path output_path = output_directory / output_file_name;
        std::ofstream output_file(output_path, std::ios::app);
        if (!output_file.is_open()) {
            std::cerr << "Could not open final output file: " << output_path << std::endl;
            return false;
        }
        for (const auto& record : records) {
            output_file << record.first << '\t' << record.second << '\n';
        }
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "Failed to append final output. Reason: " << ex.what() << std::endl;
        return false;
    }
}

bool FileManager::reset_final_output_file(
    const fs::path& output_directory,
    const std::string& output_file_name) const {
    try {
        fs::path output_path = output_directory / output_file_name;
        std::ofstream output_file(output_path, std::ios::trunc);
        return output_file.is_open();
    }
    catch (const std::exception& ex) {
        std::cerr << "Failed to reset output file. Reason: " << ex.what() << std::endl;
        return false;
    }
}

bool FileManager::create_success_file(const fs::path& output_directory) const {
    try {
        fs::path success_path = output_directory / "SUCCESS";
        std::ofstream success_file(success_path, std::ios::trunc);
        return success_file.is_open();
    }
    catch (const std::exception& ex) {
        std::cerr << "Failed to create SUCCESS file. Reason: " << ex.what() << std::endl;
        return false;
    }
}
