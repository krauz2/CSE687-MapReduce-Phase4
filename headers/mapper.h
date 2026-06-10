#pragma once

#include "file_manager.h"
#include "phase3_config.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

// Phase 3 Mapper:
// - still maps one line at a time
// - now partitions intermediate results by reducer id
// - each mapper process generates R output files
class Mapper {
public:
    Mapper(
        FileManager& file_manager,
        const std::filesystem::path& temp_directory,
        int mapper_id,
        int reducer_count,
        std::size_t max_buffer_size = 1000);

    bool map(const std::string& file_name, const std::string& raw_line);
    bool flush_buffer();

private:
    std::vector<std::string> tokenize_and_normalize(const std::string& raw_line) const;
    bool export_record(const std::string& key, int value);

    FileManager& file_manager_;
    std::filesystem::path temp_directory_;
    int mapper_id_;
    int reducer_count_;
    std::vector<std::vector<std::pair<std::string, int>>> partition_buffers_;
    std::size_t max_buffer_size_;
    std::size_t total_buffered_records_;
    Phase3Config config_;
};