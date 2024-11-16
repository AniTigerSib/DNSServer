#include <iostream>

#include "server/server.h"
#include "logger/logger.h"
#include "utils.h"

int main() {
    Logger *logger = nullptr;
    DNSServer *server = nullptr;
    boost::asio::io_context io_context;

    try {
        logger = new Logger("application.log", 512);
        auto future = logger->start();

        // Ждём успешной инициализации или ошибки
        future.get();

        std::stringstream ss;
        get_cooked_log_string(ss) << "Logger started." << std::endl;
        std::cout << ss.str();

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);

        std::string upstream_dns = "8.8.8.8";

        server = new DNSServer(8083, io_context, upstream_dns, *logger);

        signals.async_wait(
            [&](const boost::system::error_code& ec, int signal_number) {
                if (!ec) {
                    std::stringstream ss;
                    get_cooked_log_string(ss) 
                        << "Received signal " << signal_number 
                        << ". Stopping server..." << std::endl;
                    std::cout << ss.str();

                    // Останавливаем сервер
                    server->stop();
                    
                    // Останавливаем io_context
                    io_context.stop();
                    
                    // Останавливаем логгер
                    logger->stop();
                }
            });

        server->start();

        ss.clear();
        get_cooked_log_string(ss) << "Server started." << std::endl;
        std::cout << ss.str();

        io_context.run();
        
        std::stringstream final_ss;
        get_cooked_log_string(final_ss) << "Application shutdown complete." << std::endl;
        std::cout << final_ss.str();
    } catch (const std::exception& e) {
        // Останавливаем сервер
        if (server) 
            server->stop();
        
        // Останавливаем io_context
        io_context.stop();
        
        // Останавливаем логгер
        if (logger)
            logger->stop();

        std::stringstream ss;
        get_cooked_log_string(ss) << "Error: " << e.what() << std::endl;
        std::cerr << ss.str();
        return 1;
    }
    return 0;
}