#pragma once

#include <filesystem>
#include <string>
#include <utility>
#include <vector> 

class FileManager {
public:
    bool directory_exists(const std::filesystem::path& directory) const;
    bool ensure_directory(const std::filesystem::path& directory) const;
    bool clear_directory_contents(const std::filesystem::path& directory) const;
    bool remove_file_if_exists(const std::filesystem::path& file_path) const;
    bool create_empty_file(const std::filesystem::path& file_path) const;

    std::vector<std::filesystem::path> get_input_files(const std::filesystem::path& input_directory) const;
    std::vector<std::string> read_all_lines(const std::filesystem::path& file_path) const;

    bool append_records_to_file(
        const std::filesystem::path& file_path,
        const std::vector<std::pair<std::string, int>>& records) const;

    bool append_final_results(
        const std::filesystem::path& output_directory,
        const std::string& output_file_name,
        const std::vector<std::pair<std::string, int>>& records) const;

    bool reset_final_output_file(
        const std::filesystem::path& output_directory,
        const std::string& output_file_name) const;

    bool create_success_file(const std::filesystem::path& output_directory) const;
};
