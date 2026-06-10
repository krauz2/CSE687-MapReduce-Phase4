#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

class Logger {
public:
    enum class Level {
        info,
        warning,
        error,
        fatal
    };

    static Logger& instance();

    bool initialize(const std::filesystem::path& log_file_path, bool truncate_existing = true);
    void shutdown();

    void log(Level level, const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);

private:
    Logger() = default;
    std::string level_to_string(Level level) const;

    std::mutex mutex_;
    std::ofstream log_file_;
};
