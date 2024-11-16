#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <iomanip>

inline std::stringstream& get_cooked_log_string(std::stringstream& ss) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count()
       << ": ";

    return ss;
}

#endif