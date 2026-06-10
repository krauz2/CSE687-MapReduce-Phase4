#pragma once

#include <filesystem>
#include <string>

// Central place to change default names and naming patterns for Phase 3.
// Directories are still configurable from the command line in Executive.
struct Phase3Config {
    std::string final_output_file_name = "word_counts.txt";
    std::string success_file_name = "SUCCESS";
    std::string logs_directory_name = "logs";
    std::string controller_log_name = "controller.log";
    std::string mapper_log_prefix = "mapper_";
    std::string reducer_log_prefix = "reducer_";
    std::string reducer_output_prefix = "reducer_";
    std::string partition_prefix = "mapper_";
    std::string partition_middle = "_reduce_";
    std::string partition_extension = ".txt";
    std::string mapper_worker_default_name = "mapper_worker.exe";
    std::string reducer_worker_default_name = "reducer_worker.exe";
    int default_reducer_count = 2;

    std::filesystem::path logs_directory(const std::filesystem::path& output_directory) const {
        return output_directory / logs_directory_name;
    }

    std::filesystem::path controller_log_path(const std::filesystem::path& output_directory) const {
        return logs_directory(output_directory) / controller_log_name;
    }

    std::filesystem::path mapper_log_path(const std::filesystem::path& output_directory, int mapper_id) const {
        return logs_directory(output_directory) / (mapper_log_prefix + std::to_string(mapper_id) + ".log");
    }

    std::filesystem::path reducer_log_path(const std::filesystem::path& output_directory, int reducer_id) const {
        return logs_directory(output_directory) / (reducer_log_prefix + std::to_string(reducer_id) + ".log");
    }

    std::filesystem::path reducer_output_path(const std::filesystem::path& output_directory, int reducer_id) const {
        return output_directory / (reducer_output_prefix + std::to_string(reducer_id) + ".txt");
    }

    std::filesystem::path mapper_partition_path(const std::filesystem::path& temp_directory, int mapper_id, int reducer_id) const {
        return temp_directory / (partition_prefix + std::to_string(mapper_id) + partition_middle + std::to_string(reducer_id) + partition_extension);
    }
};