#include "logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdarg>

namespace lisa::server {

// 静态成员初始化
std::ofstream Logger::log_file_;
LogLevel Logger::log_level_ = LogLevel::INFO;
std::mutex Logger::log_mutex_;
bool Logger::initialized_ = false;

void Logger::init(const std::string& log_file, LogLevel level) {
    std::lock_guard<std::mutex> lock(log_mutex_);

    if (initialized_) {
        warn("Logger already initialized");
        return;
    }

    log_level_ = level;

    // 如果指定了日志文件，则尝试打开
    if (!log_file.empty()) {
        log_file_.open(log_file, std::ios::app);
        if (!log_file_.is_open()) {
            std::cerr << "Failed to open log file: " << log_file << ", using console output instead" << std::endl;
        }
    }

    initialized_ = true;
    info("Logger initialized successfully");
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(log_mutex_);

    if (!initialized_) return;

    info("Logger shutting down");
    if (log_file_.is_open()) {
        log_file_.close();
    }
    initialized_ = false;
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    log_level_ = level;
    info("Log level changed to: " + level_to_string(level));
}

std::string Logger::get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::format_message(LogLevel level, const std::string& message) {
    return "[" + get_current_time() + "] [" + level_to_string(level) + "]: " + message;
}

void Logger::debug(const std::string& message) {
    if (log_level_ > LogLevel::DEBUG) return;
    std::lock_guard<std::mutex> lock(log_mutex_);
    std::string formatted = format_message(LogLevel::DEBUG, message);
    if (log_file_.is_open()) {
        log_file_ << formatted << std::endl;
    } else {
        std::cout << formatted << std::endl;
    }
}

void Logger::info(const std::string& message) {
    if (log_level_ > LogLevel::INFO) return;
    std::lock_guard<std::mutex> lock(log_mutex_);
    std::string formatted = format_message(LogLevel::INFO, message);
    if (log_file_.is_open()) {
        log_file_ << formatted << std::endl;
    } else {
        std::cout << formatted << std::endl;
    }
}

void Logger::warn(const std::string& message) {
    if (log_level_ > LogLevel::WARNING) return;
    std::lock_guard<std::mutex> lock(log_mutex_);
    std::string formatted = format_message(LogLevel::WARNING, message);
    if (log_file_.is_open()) {
        log_file_ << formatted << std::endl;
    } else {
        std::cerr << formatted << std::endl;
    }
}

void Logger::error(const std::string& message) {
    if (log_level_ > LogLevel::ERROR) return;
    std::lock_guard<std::mutex> lock(log_mutex_);
    std::string formatted = format_message(LogLevel::ERROR, message);
    if (log_file_.is_open()) {
        log_file_ << formatted << std::endl;
    } else {
        std::cerr << formatted << std::endl;
    }
}

void Logger::fatal(const std::string& message) {
    if (log_level_ > LogLevel::FATAL) return;
    std::lock_guard<std::mutex> lock(log_mutex_);
    std::string formatted = format_message(LogLevel::FATAL, message);
    if (log_file_.is_open()) {
        log_file_ << formatted << std::endl;
    } else {
        std::cerr << formatted << std::endl;
    }
}

} // namespace lisa::server