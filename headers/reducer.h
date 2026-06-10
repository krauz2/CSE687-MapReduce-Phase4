/*
Reduce Class: The reduce class will have a method reduce() that will take a string with one key and an iterator of integers. 
he reduce function should sum all the values in the iterator and then call an export function (different from the Map class). 
The export function will take two parameters, the key and resulting reduced value, and write the result out to the output directory. 
Upon success an additional empty file SUCCESS will be written out to the output directory. 
The reduce method itself should not directly deal with any File IO. 
*/

// Data input format: ("a", [1, 1, 1]), ("the", [1, 1]) etc.
// After reducing: ("a", 3), ("the", 2) etc.

#pragma once                // Prevent duplicate inclusions of header files during compilation.
#include "file_manager.h"   // Need FileManager to write final output and SUCCESS files.
#include <filesystem>       // Used for working with file directories.
#include <string>
#include <utility>          // Used for std::pair.
#include <vector>           // Dynamically sized arrays.

class Reducer {
public:
    // Constructor, simpler way to define class parameters
    Reducer(
        FileManager& fileManager,                       // Need FileManager to write final output and SUCCESS.
        const std::filesystem::path& output_directory,  // Where final output file is written to.
        const std::string& output_file_name,            // Name of final output file.
        std::size_t max_buffer_size = 500);             // Buffer size.

    // Reduce method takes a key and its values, sums values, then passes key and sum to export_record().
    // e.g. key = "a"; values = [1, 1, 1]; sum = 3
    bool reduce(const std::string& key, const std::vector<int>& values);

    // Assuming no more appends to output file after this:
    // Make sure all contents of buffer have been appended to output file, then create SUCCESS file.
    bool finish();

private:
    // Add key and sum to buffer. Check if buffer is full.
    bool export_record(const std::string& key, int reduced_value);

    // Append contents of buffer into output file, then empty buffer.
    bool flush_buffer();

    FileManager& file_manager_;                         // Shared FileManager from Workflow.
    std::filesystem::path output_directory_;            // File directory where final output file will be saved.
    std::string output_file_name_;                      // Name of final output file.
    std::vector<std::pair<std::string, int>> buffer_;   // Buffer.
    std::size_t max_buffer_size_;                       // Buffer size.
};
