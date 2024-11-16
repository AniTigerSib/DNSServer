#include "logger.h"

std::string Logger::formatMessage(const std::string& message) {
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

void Logger::processQueue() {
    std::ofstream log_file(filename_, std::ios::app);
        
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
            log_file.flush();
        }
    }
}