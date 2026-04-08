#pragma once

#include <string>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <mutex>

namespace tradex {
namespace framework {

// Log levels
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Fatal = 4
};

class Logger {
public:
    static Logger& instance();

    void setLevel(LogLevel level);
    void setPattern(const std::string& pattern);

    // File logging
    bool enableFileLog(const std::string& filename);
    void setMaxFileSize(size_t max_size);
    void setMaxFiles(int max_files);

    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);

    // Template method for formatted logging
    template<typename... Args>
    void logf(LogLevel level, const char* fmt, Args&&... args) {
        char buffer[4096];
        snprintf(buffer, sizeof(buffer), fmt, std::forward<Args>(args)...);
        log(level, buffer);
    }

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void rotateLogs();

    LogLevel level_;
    std::string pattern_;
    std::mutex mutex_;

    // File logging
    std::string log_file_;
    std::ofstream file_stream_;
    size_t max_file_size_;
    int max_files_;
    std::mutex file_mutex_;

    std::string formatMessage(LogLevel level, const std::string& message);
    const char* levelToString(LogLevel level);
};

// Convenience macros
#define LOG_DEBUG(msg) tradex::framework::Logger::instance().debug(msg)
#define LOG_INFO(msg) tradex::framework::Logger::instance().info(msg)
#define LOG_WARNING(msg) tradex::framework::Logger::instance().warning(msg)
#define LOG_ERROR(msg) tradex::framework::Logger::instance().error(msg)
#define LOG_FATAL(msg) tradex::framework::Logger::instance().fatal(msg)

#define LOG_DEBUGF(fmt, ...) \
    tradex::framework::Logger::instance().logf(tradex::framework::LogLevel::Debug, fmt, ##__VA_ARGS__)
#define LOG_INFOF(fmt, ...) \
    tradex::framework::Logger::instance().logf(tradex::framework::LogLevel::Info, fmt, ##__VA_ARGS__)
#define LOG_WARNINGF(fmt, ...) \
    tradex::framework::Logger::instance().logf(tradex::framework::LogLevel::Warning, fmt, ##__VA_ARGS__)
#define LOG_ERRORF(fmt, ...) \
    tradex::framework::Logger::instance().logf(tradex::framework::LogLevel::Error, fmt, ##__VA_ARGS__)
#define LOG_FATALF(fmt, ...) \
    tradex::framework::Logger::instance().logf(tradex::framework::LogLevel::Fatal, fmt, ##__VA_ARGS__)

} // namespace framework
} // namespace tradex