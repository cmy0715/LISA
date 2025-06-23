#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <ctime>

namespace lisa::server {

// 日志级别
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    // 初始化日志系统
    static void init(const std::string& log_file = "", LogLevel level = LogLevel::INFO);

    // 关闭日志系统
    static void shutdown();

    // 日志输出方法
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    static void fatal(const std::string& message);

    // 设置日志级别
    static void set_level(LogLevel level);

private:
    static std::ofstream log_file_;
    static LogLevel log_level_;
    static std::mutex log_mutex_;
    static bool initialized_;

    // 格式化日志消息
    static std::string format_message(LogLevel level, const std::string& message);

    // 获取当前时间字符串
    static std::string get_current_time();

    // 将日志级别转换为字符串
    static std::string level_to_string(LogLevel level);
};

} // namespace lisa::server

#endif // LOGGER_H