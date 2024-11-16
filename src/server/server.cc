#include "server.h"
#include <iostream>
#include "../utils.h"

void DNSServer::handleRequest(std::size_t bytes_recvd) {
    std::vector<char> forward_buffer(data_.begin(), 
                                       data_.begin() + bytes_recvd);

    forward_socket_.async_send_to(
        boost::asio::buffer(forward_buffer), forward_endpoint_,
        [](boost::system::error_code ec, std::size_t) {
            if (ec) {
                std::cerr << "Forward error: " << ec.message() << std::endl;
            }
        });

    try {
        std::string domain_name = DNSNameExtractor::extractDomainName(data_, bytes_recvd);
        std::string log_message = sender_endpoint_.address().to_string() + " " + domain_name;

        logger_ << log_message; 
    } catch (const LoggerException& e) {
        std::stringstream ss;
        get_cooked_log_string(ss) << "Error: " << e.what() << std::endl;

        std::cerr << ss.str(); // TODO: Create logging system
    } // Логгирования не происходит, но пакет отправляется
}

std::string DNSServer::DNSNameExtractor::extractDomainName(const std::array<uint8_t, MAX_DNS_PACKET_SIZE> buffer, size_t buffer_size, size_t offset) {
    if (buffer_size < offset) {
        throw std::runtime_error("Buffer too small");
    }

    // Предварительно резервируем память для среднего размера домена
    std::string result;
    result.reserve(64);
    
    size_t pos = offset;
    uint8_t jumps = 0;
    const uint8_t max_jumps = 10; // Защита от циклических ссылок
    
    while (pos < buffer_size) {
        // Получаем длину текущей метки
        uint8_t label_length = buffer[pos];
        
        // Проверяем на конец имени
        if (label_length == 0) {
            break;
        }
        
        // Проверяем на сжатое имя
        if ((label_length & DNS_COMPRESSION_MASK) == DNS_COMPRESSION_FLAG) {
            if (pos + 1 >= buffer_size) {
                throw std::runtime_error("Invalid compression pointer");
            }
            
            if (++jumps > max_jumps) {
                throw std::runtime_error("Too many compression pointers");
            }
            
            // Вычисляем смещение для сжатого имени
            size_t new_pos = ((label_length & ~DNS_COMPRESSION_MASK) << 8) | buffer[pos + 1];
            if (new_pos >= pos) {
                throw std::runtime_error("Invalid forward compression pointer");
            }
            pos = new_pos;
            continue;
        }
        
        // Проверяем корректность длины метки
        if (label_length > MAX_LABEL_LENGTH) {
            throw std::runtime_error("Label too long");
        }
        
        if (pos + 1 + label_length > buffer_size) {
            throw std::runtime_error("Label exceeds buffer");
        }
        
        // Добавляем разделитель, если это не первая метка
        if (!result.empty()) {
            result.push_back('.');
        }
        
        // Добавляем символы метки
        result.append(reinterpret_cast<const char*>(&buffer[pos + 1]), label_length);
        
        // Проверяем общую длину имени
        if (result.length() > MAX_DNS_LENGTH) {
            throw std::runtime_error("Domain name too long");
        }
        
        pos += label_length + 1;
    }
    
    return result;
}