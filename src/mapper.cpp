#include "mapper.h"

#include <cctype>
#include <functional>
#include <sstream>

Mapper::Mapper(
    FileManager& file_manager,
    const std::filesystem::path& temp_directory,
    int mapper_id,
    int reducer_count,
    std::size_t max_buffer_size)
    : file_manager_(file_manager),
    temp_directory_(temp_directory),
    mapper_id_(mapper_id),
    reducer_count_(reducer_count),
    partition_buffers_(static_cast<std::size_t>(reducer_count)),
    max_buffer_size_(max_buffer_size),
    total_buffered_records_(0) {

    // Phase 3 requirement: each mapper process generates R output files.
    for (int reducer_id = 0; reducer_id < reducer_count_; ++reducer_id) {
        file_manager_.create_empty_file(
            config_.mapper_partition_path(temp_directory_, mapper_id_, reducer_id));
    }
}

bool Mapper::map(const std::string& file_name, const std::string& raw_line) {
    (void)file_name;

    const std::vector<std::string> tokens = tokenize_and_normalize(raw_line);

    for (const auto& token : tokens) {
        if (!export_record(token, 1)) {
            return false;
        }
    }

    return true;
}

std::vector<std::string> Mapper::tokenize_and_normalize(const std::string& raw_line) const {
    std::string cleaned_line;
    cleaned_line.reserve(raw_line.size());

    for (unsigned char ch : raw_line) {
        if (std::isalnum(ch)) {
            cleaned_line.push_back(static_cast<char>(std::tolower(ch)));
        }
        else {
            cleaned_line.push_back(' ');
        }
    }

    std::stringstream stream(cleaned_line);
    std::vector<std::string> tokens;
    std::string token;

    while (stream >> token) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}

bool Mapper::export_record(const std::string& key, int value) {
    const std::size_t bucket =
        std::hash<std::string>{}(key) % static_cast<std::size_t>(reducer_count_);

    partition_buffers_[bucket].push_back({ key, value });
    ++total_buffered_records_;

    if (total_buffered_records_ >= max_buffer_size_) {
        return flush_buffer();
    }

    return true;
}

bool Mapper::flush_buffer() {
    if (total_buffered_records_ == 0) {
        return true;
    }

    for (int reducer_id = 0; reducer_id < reducer_count_; ++reducer_id) {
        auto& buffer = partition_buffers_[static_cast<std::size_t>(reducer_id)];

        if (buffer.empty()) {
            continue;
        }

        const std::filesystem::path partition_path =
            config_.mapper_partition_path(temp_directory_, mapper_id_, reducer_id);

        const bool ok = file_manager_.append_records_to_file(partition_path, buffer);
        if (!ok) {
            return false;
        }

        buffer.clear();
    }

    total_buffered_records_ = 0;
    return true;
}