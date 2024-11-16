#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstring>

using boost::asio::ip::udp;

// constexpr uint16_t DNS_PORT = 8080;
constexpr uint16_t DNS_PORT = 53; // Стандартный DNS-порт
constexpr size_t MAX_DNS_PACKET_SIZE = 512; // Максимальный размер DNS пакета

std::queue<std::vector<uint8_t>> log_queue;
std::mutex queue_mutex;
std::condition_variable queue_cv;

void log_worker() {
    while (true) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait(lock, [] { return !log_queue.empty(); });

        // Забираем пакет из очереди
        auto packet = log_queue.front();
        log_queue.pop();
        lock.unlock();

        // Логгирование пакета
        std::cout << "Logged DNS packet: ";
        for (uint8_t byte : packet) {
            std::cout << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::endl;
    }
}

class DNSServer {
public:
    DNSServer(boost::asio::io_context& io_context, const std::string& upstream_dns)
        : socket_(io_context, udp::endpoint(udp::v4(), DNS_PORT)),
          resolver_(io_context), upstream_dns_(upstream_dns) {}

    void start() { receive(); }

private:
    void receive() {
        socket_.async_receive_from(
            boost::asio::buffer(data_, MAX_DNS_PACKET_SIZE), sender_endpoint_,
            [this](boost::system::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0) {
                    handle_request(bytes_recvd);
                }
                receive();
            });
    }

    void handle_request(std::size_t bytes_recvd) {
        // Сохраняем копию пакета в очередь
        std::vector<uint8_t> packet_copy(data_, data_ + bytes_recvd);
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            log_queue.push(packet_copy);
        }
        queue_cv.notify_one();

        // Отправляем оригинальный запрос на upstream DNS сервер
        auto query = std::make_shared<std::vector<uint8_t>>(data_, data_ + bytes_recvd);
        udp::resolver::query resolver_query(upstream_dns_, "53");
        resolver_.async_resolve(
            resolver_query,
            [this, query](const boost::system::error_code& ec, udp::resolver::iterator it) {
                if (!ec) {
                    forward_request(query, it->endpoint());
                }
            });
    }

    void forward_request(std::shared_ptr<std::vector<uint8_t>> query, const udp::endpoint& upstream_endpoint) {
        socket_.async_send_to(
            boost::asio::buffer(*query), upstream_endpoint,
            [this, query](boost::system::error_code ec, std::size_t bytes_sent) {
                if (!ec) {
                    // Ожидание ответа от upstream DNS сервера
                    receive_upstream_response();
                }
            });
    }

    void receive_upstream_response() {
        socket_.async_receive_from(
            boost::asio::buffer(data_, MAX_DNS_PACKET_SIZE), upstream_response_endpoint_,
            [this](boost::system::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0) {
                    // Отправляем ответ клиенту
                    socket_.async_send_to(
                        boost::asio::buffer(data_, bytes_recvd), sender_endpoint_,
                        [](boost::system::error_code, std::size_t) {});
                }
            });
    }

    udp::socket socket_;
    udp::resolver resolver_;
    udp::endpoint sender_endpoint_;
    udp::endpoint upstream_response_endpoint_;
    std::string upstream_dns_;
    uint8_t data_[MAX_DNS_PACKET_SIZE];
};

int main() {
    try {
        boost::asio::io_context io_context;

        std::string upstream_dns = "8.8.8.8"; // Внешний DNS сервер (например, Google Public DNS)

        // Запускаем логгирование в отдельном потоке
        std::thread log_thread(log_worker);

        DNSServer server(io_context, upstream_dns);
        server.start();

        io_context.run();

        log_thread.join();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
