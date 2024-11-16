#include <fstream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger {
public:
    explicit Logger(const std::string& filename) : 
        filename_(filename), 
        running_(false) {}

    ~Logger() {
        stop();
    }

    // Начало работы логгера
    void start() {
        running_ = true;
        worker_thread_ = std::thread(&Logger::processQueue, this);
    }

    // Остановка работы логгера
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
        }
        condition_.notify_one();
        
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    // Добавление сообщения в очередь
    void log(const std::string& message) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            message_queue_.push(formatMessage(message));
        }
        condition_.notify_one();
    }

    void operator<< (const std::string& message) { log(message); }

private:
    std::string filename_;
    std::queue<std::string> message_queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread worker_thread_;
    bool running_;

    // Форматирование сообщения с временной меткой
    std::string formatMessage(const std::string& message);

    // Основной цикл обработки очереди сообщений
    void processQueue();
};