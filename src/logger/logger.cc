#include "logger.h"

#include <chrono>
#include <fstream>
#include <iomanip>

std::string Logger::format_message(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count()
       << ": " << message;
    
    return ss.str();
}

void Logger::process_queue() {
    std::ofstream log_file(filename_, std::ios::app);
    
    if (!log_file.is_open()) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            error_occurred_ = true;
        }

        error_promise_.set_exception(
            std::make_exception_ptr(
                LoggerException("Failed to open log file: " + filename_)
            )
        );
        return;
    }

    try {
        error_promise_.set_value(); // Сигнализируем об успешном открытии файла

        while (true) {
            std::string message;

            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] {
                    return !message_queue_.empty() || !running_;
                });

                if (!running_ && message_queue_.empty()) {
                    break;
                }

                if (!message_queue_.empty()) {
                    message = std::move(message_queue_.front());
                    message_queue_.pop();
                }
            }

            if (!message.empty()) {
                log_file << message << std::endl;
                if (!log_file) {
                    throw LoggerException("Failed to write to log file");
                }
                log_file.flush();
            }
        }
    }
    catch (const std::exception& e) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            error_occurred_ = true;
        }
        // В случае если promise уже был использован (что означает, что файл был успешно открыт),
        // исключение будет проигнорировано
        try {
            error_promise_.set_exception(std::current_exception());
        }
        catch (const std::future_error&) {}
    }
}