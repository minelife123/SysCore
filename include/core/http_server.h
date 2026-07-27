#ifndef SYSCORE_HTTP_SERVER_H
#define SYSCORE_HTTP_SERVER_H

#include "core/network_manager.h"
#include <string>
#include <functional>
#include <thread>
#include <atomic>

namespace SysCore::Network {

    using HttpHandler = std::function<std::string(const std::string& method, const std::string& path)>;

    class TelemetryHttpServer {
    private:
        WsaInitScope m_wsaScope;
        SOCKET m_listenSocket{INVALID_SOCKET};
        uint16_t m_port{8080};
        std::atomic<bool> m_running{false};
        std::jthread m_serverThread;
        HttpHandler m_handler;

        void ServerLoop();

    public:
        TelemetryHttpServer() = default;
        ~TelemetryHttpServer() { Stop(); }

        TelemetryHttpServer(const TelemetryHttpServer&) = delete;
        TelemetryHttpServer& operator=(const TelemetryHttpServer&) = delete;

        bool Start(uint16_t port, HttpHandler handler);
        void Stop();

        [[nodiscard]] bool IsRunning() const noexcept { return m_running; }
    };

} // namespace SysCore::Network

#endif // SYSCORE_HTTP_SERVER_H
