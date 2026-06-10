/*
Reduce Class: The reduce class will have a method reduce() that will take a string with one key and an iterator of integers. 
he reduce function should sum all the values in the iterator and then call an export function (different from the Map class). 
The export function will take two parameters, the key and resulting reduced value, and write the result out to the output directory. 
Upon success an additional empty file SUCCESS will be written out to the output directory. 
The reduce method itself should not directly deal with any File IO. 
*/

// Data input format: ("a", [1, 1, 1]), ("the", [1, 1]) etc.
// After reducing: ("a", 3), ("the", 2) etc.

#include "reducer.h"
#include <numeric>  // Needed for std::accumulate, to add values together.

// Use constructor to intialize parameters for Reducer class 
Reducer::Reducer(
    FileManager& fileManager,                       // Need FileManager to write final output and SUCCESS.
    const std::filesystem::path& output_directory,  // File directory where final output file will be saved.
    const std::string& output_file_name,            // File name for final output file.
    std::size_t max_buffer_size)                    // Buffer size.
    : file_manager_(fileManager),
      output_directory_(output_directory),
      output_file_name_(output_file_name),
      max_buffer_size_(max_buffer_size) { 
}

// key - "a", values - [1, 1, 1]
bool Reducer::reduce(const std::string& key, const std::vector<int>& values) {
    // Add all integers in the values vector together e.g. [1, 1, 1] becomes 3.
    const int sum = std::accumulate(values.begin(), values.end(), 0);

    // Pass key and sum to export_record() function
    return export_record(key, sum);
}

// Add key and sum to buffer. If buffer is full, append buffer contents to final output file.
bool Reducer::export_record(const std::string& key, int reduced_value) {
    // Add key and sum to buffer
    buffer_.push_back({ key, reduced_value });

    // If buffer is full, append contents of buffer into the final output file.
    if (buffer_.size() >= max_buffer_size_) {
        return flush_buffer();
    }

    return true;
}

// Append contents of buffer to final output file, then empty buffer.
bool Reducer::flush_buffer() {
    // If buffer is empty, nothing to write.
    if (buffer_.empty()) {
        return true;
    }

    // Ask FileManager to append buffer contents to final output file.
    const bool ok = file_manager_.append_final_results(output_directory_, output_file_name_, buffer_);
    if (!ok) {
        return false;  // Stop if writing fails.
    }

    buffer_.clear();   // Clear buffer once data is appended to final output file.
    return true;
}

// Assuming no more appends to final output file after this:
// Make sure all contents of buffer have been appended to final output file, then create SUCCESS file.
bool Reducer::finish() {
    // Make sure buffer is empty.
    if (!flush_buffer()) {
        return false;
    }

    // Create SUCCESS file.
    return file_manager_.create_success_file(output_directory_);
}
