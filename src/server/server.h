#include <boost/asio.hpp>
#include <cstdint>
#include "../logger/logger.h"

using boost::asio::ip::udp;

constexpr size_t MAX_DNS_PACKET_SIZE = 512; // Максимальный размер DNS пакета

class DNSServer {
public:
    DNSServer(const uint16_t dns_port, boost::asio::io_context& io_context, const std::string& upstream_dns, Logger &logger)
        : socket_(io_context, udp::endpoint(udp::v4(), dns_port)),
          resolver_(io_context), upstream_dns_(upstream_dns), logger_(logger) {}

    void start() { receive(); }

private:
    udp::socket socket_;
    udp::resolver resolver_;
    udp::endpoint sender_endpoint_;
    udp::endpoint upstream_response_endpoint_;
    std::string upstream_dns_;
    uint8_t data_[MAX_DNS_PACKET_SIZE];
    Logger &logger_;

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

    void handle_request(std::size_t bytes_recvd);

    class DNSNameExtractor {
    private:
        // Константы для DNS пакета
        static constexpr uint8_t DNS_COMPRESSION_MASK = 0xC0;
        static constexpr uint8_t DNS_COMPRESSION_FLAG = 0xC0;
        static constexpr size_t MAX_DNS_LENGTH = 255;
        static constexpr size_t MAX_LABEL_LENGTH = 63;

    public:
        static std::string extract_domain_name(const uint8_t* buffer, size_t buffer_size, size_t offset = 12);
    };
};