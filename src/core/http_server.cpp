#include "core/http_server.h"
#include "core/logger.h"
#include <sstream>

namespace SysCore::Network {

    bool TelemetryHttpServer::Start(uint16_t port, HttpHandler handler) {
        if (m_running) return false;
        m_port = port;
        m_handler = std::move(handler);

        m_listenSocket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);
        if (m_listenSocket == INVALID_SOCKET) {
            Logging::LoggerCore::Instance().Error("TelemetryHttpServer: Failed to create socket");
            return false;
        }

        BOOL reuseAddr = TRUE;
        ::setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuseAddr), sizeof(reuseAddr));

        sockaddr_in service{};
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = ::htonl(INADDR_ANY);
        service.sin_port = ::htons(port);

        if (::bind(m_listenSocket, reinterpret_cast<SOCKADDR*>(&service), sizeof(service)) == SOCKET_ERROR) {
            // Fallback to ephemeral OS assigned port 0 if requested port is unavailable
            service.sin_port = ::htons(0);
            if (::bind(m_listenSocket, reinterpret_cast<SOCKADDR*>(&service), sizeof(service)) == SOCKET_ERROR) {
                Logging::LoggerCore::Instance().Warning("TelemetryHttpServer: Failed to bind socket");
                ::closesocket(m_listenSocket);
                m_listenSocket = INVALID_SOCKET;
                return false;
            }
        }

        int nameLen = sizeof(service);
        if (::getsockname(m_listenSocket, reinterpret_cast<SOCKADDR*>(&service), &nameLen) != SOCKET_ERROR) {
            m_port = ::ntohs(service.sin_port);
        }

        if (::listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            ::closesocket(m_listenSocket);
            m_listenSocket = INVALID_SOCKET;
            return false;
        }

        m_running = true;
        m_serverThread = std::jthread([this]() { ServerLoop(); });

        Logging::LoggerCore::Instance().Info("TelemetryHttpServer: Listening for REST metrics requests on http://127.0.0.1:" + std::to_string(m_port));
        return true;
    }

    void TelemetryHttpServer::ServerLoop() {
        while (m_running) {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(m_listenSocket, &readSet);

            timeval timeout{.tv_sec = 0, .tv_usec = 100000}; // 100ms
            int selectResult = ::select(0, &readSet, nullptr, nullptr, &timeout);

            if (selectResult > 0 && FD_ISSET(m_listenSocket, &readSet)) {
                SOCKET clientSocket = ::accept(m_listenSocket, nullptr, nullptr);
                if (clientSocket != INVALID_SOCKET) {
                    char requestBuf[1024]{0};
                    int bytesReceived = ::recv(clientSocket, requestBuf, sizeof(requestBuf) - 1, 0);

                    if (bytesReceived > 0) {
                        std::string requestStr(requestBuf, bytesReceived);
                        std::istringstream stream(requestStr);
                        std::string method, path;
                        stream >> method >> path;

                        std::string jsonBody = m_handler ? m_handler(method, path) : "{\"status\":\"ok\"}";
                        std::string response = 
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/json\r\n"
                            "Access-Control-Allow-Origin: *\r\n"
                            "Content-Length: " + std::to_string(jsonBody.size()) + "\r\n"
                            "Connection: close\r\n\r\n" + jsonBody;

                        ::send(clientSocket, response.data(), static_cast<int>(response.size()), 0);
                    }
                    ::closesocket(clientSocket);
                }
            }
        }
    }

    void TelemetryHttpServer::Stop() {
        if (m_running) {
            m_running = false;
            if (m_listenSocket != INVALID_SOCKET) {
                ::closesocket(m_listenSocket);
                m_listenSocket = INVALID_SOCKET;
            }
            if (m_serverThread.joinable()) {
                m_serverThread.join();
            }
            Logging::LoggerCore::Instance().Info("TelemetryHttpServer stopped.");
        }
    }

} // namespace SysCore::Network
