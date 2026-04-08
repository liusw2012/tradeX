#include "tradex/framework/log.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace tradex {
namespace framework {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() : level_(LogLevel::Info), pattern_("[%timestamp%] [%level%] %message%"),
    max_file_size_(10 * 1024 * 1024), max_files_(5) {}

Logger::~Logger() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

void Logger::setPattern(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(mutex_);
    pattern_ = pattern;
}

bool Logger::enableFileLog(const std::string& filename) {
    std::lock_guard<std::mutex> lock(file_mutex_);

    log_file_ = filename;

    // Open file in append mode
    file_stream_.open(filename, std::ios::app);
    if (!file_stream_.is_open()) {
        return false;
    }

    return true;
}

void Logger::setMaxFileSize(size_t max_size) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    max_file_size_ = max_size;
}

void Logger::setMaxFiles(int max_files) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    max_files_ = max_files;
}

void Logger::rotateLogs() {
    if (log_file_.empty() || !file_stream_.is_open()) {
        return;
    }

    // Check file size
    file_stream_.flush();
    size_t current_size = file_stream_.tellp();
    if (current_size < max_file_size_) {
        return;
    }

    // Close current file
    file_stream_.close();

    // Rotate existing files
    for (int i = max_files_ - 1; i >= 1; --i) {
        std::string old_file = log_file_ + "." + std::to_string(i);
        std::string new_file = log_file_ + "." + std::to_string(i + 1);

        // Remove old new file if exists
        std::remove(new_file.c_str());
        // Rename old file
        std::rename(old_file.c_str(), new_file.c_str());
    }

    // Rename current to .1
    std::string first_file = log_file_ + ".1";
    std::remove(first_file.c_str());
    std::rename(log_file_.c_str(), first_file.c_str());

    // Open new file
    file_stream_.open(log_file_, std::ios::app);
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

std::string Logger::formatMessage(LogLevel level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    std::string result = pattern_;
    size_t pos;

    while ((pos = result.find("%timestamp%")) != std::string::npos) {
        result.replace(pos, 11, oss.str());
    }
    while ((pos = result.find("%level%")) != std::string::npos) {
        result.replace(pos, 7, levelToString(level));
    }
    while ((pos = result.find("%message%")) != std::string::npos) {
        result.replace(pos, 9, message);
    }

    return result;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < level_) return;

    std::string formatted = formatMessage(level, message);

    // Output to console
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cerr << formatted << std::endl;
    }

    // Output to file
    if (file_stream_.is_open()) {
        std::lock_guard<std::mutex> lock(file_mutex_);
        if (file_stream_) {
            file_stream_ << formatted << std::endl;
            file_stream_.flush();

            // Check for rotation
            if (file_stream_.tellp() > static_cast<std::streampos>(max_file_size_)) {
                rotateLogs();
            }
        }
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::Debug, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::Info, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::Warning, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::Error, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::Fatal, message);
}

} // namespace framework
} // namespace tradex