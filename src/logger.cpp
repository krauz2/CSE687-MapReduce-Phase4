#include "logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

bool Logger::initialize(const std::filesystem::path& log_file_path, bool truncate_existing) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (log_file_.is_open()) {
        log_file_.close();
    }

    const auto mode = truncate_existing ? std::ios::trunc : std::ios::app;
    log_file_.open(log_file_path, mode);
    return log_file_.is_open();
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void Logger::log(Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
#ifdef _WIN32
    localtime_s(&local_time, &now_time);
#else
    local_time = *std::localtime(&now_time);
#endif

    std::ostringstream line;
    line << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S")
         << " [" << level_to_string(level) << "] "
         << message;

    if (level == Level::error || level == Level::fatal) {
        std::cerr << line.str() << std::endl;
    } else {
        std::cout << line.str() << std::endl;
    }

    if (log_file_.is_open()) {
        log_file_ << line.str() << std::endl;
        log_file_.flush();
    }
}

void Logger::info(const std::string& message) { log(Level::info, message); }
void Logger::warning(const std::string& message) { log(Level::warning, message); }
void Logger::error(const std::string& message) { log(Level::error, message); }
void Logger::fatal(const std::string& message) { log(Level::fatal, message); }

std::string Logger::level_to_string(Level level) const {
    switch (level) {
    case Level::info: return "INFO";
    case Level::warning: return "WARNING";
    case Level::error: return "ERROR";
    case Level::fatal: return "FATAL";
    default: return "UNKNOWN";
    }
}
