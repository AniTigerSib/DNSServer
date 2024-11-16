#include <exception>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <future>

class LoggerException : public std::exception {
public:
    explicit LoggerException(const std::string& message) : message_(message) {}
    explicit LoggerException(const char* message) : message_(message) {}

    virtual const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};

class Logger {
public:
    explicit Logger(const std::string& filename) : 
        filename_(filename), 
        running_(false),
        error_occurred_(false) {}

    ~Logger() {
        stop();
    }

    // Начало работы логгера с возвратом future для отслеживания ошибок
    std::future<void> start() {
        running_ = true;
        error_promise_ = std::promise<void>();
        auto future = error_promise_.get_future();

        worker_thread_ = std::thread(&Logger::process_queue, this);
        return future;
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
            if (error_occurred_) {
                throw LoggerException("Logger is in error state");
            }
            message_queue_.push(format_message(message));
        }
        condition_.notify_one();
    }

    void operator<< (const std::string& message) { log(message); }

    bool hasError() const {
        return error_occurred_;
    }

private:
    std::string filename_;
    std::queue<std::string> message_queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread worker_thread_;
    bool running_;
    bool error_occurred_;
    std::promise<void> error_promise_;

    // Форматирование сообщения с добавлением временной метки
    std::string format_message(const std::string& message);

    // Основной цикл обработки очереди сообщений
    void process_queue();
};