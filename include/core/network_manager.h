#ifndef SYSCORE_NETWORK_MANAGER_H
#define SYSCORE_NETWORK_MANAGER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <string_view>
#include <cstdint>
#include <memory>

#pragma comment(lib, "ws2_32.lib")

namespace SysCore::Network {

    class WsaInitScope {
    private:
        bool m_initialized{false};
    public:
        WsaInitScope() {
            WSADATA wsaData{};
            if (::WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
                m_initialized = true;
            }
        }
        ~WsaInitScope() {
            if (m_initialized) {
                ::WSACleanup();
            }
        }
        [[nodiscard]] bool IsInitialized() const noexcept { return m_initialized; }
    };

    class SocketChannel {
    private:
        SOCKET m_socket{INVALID_SOCKET};
    public:
        SocketChannel() = default;
        explicit SocketChannel(SOCKET socket) : m_socket(socket) {}
        ~SocketChannel() { Close(); }

        SocketChannel(const SocketChannel&) = delete;
        SocketChannel& operator=(const SocketChannel&) = delete;

        SocketChannel(SocketChannel&& other) noexcept : m_socket(other.m_socket) {
            other.m_socket = INVALID_SOCKET;
        }

        SocketChannel& operator=(SocketChannel&& other) noexcept {
            if (this != &other) {
                Close();
                m_socket = other.m_socket;
                other.m_socket = INVALID_SOCKET;
            }
            return *this;
        }

        bool Create(int af = AF_INET, int type = SOCK_STREAM, int protocol = IPPROTO_TCP);
        bool Connect(std::string_view host, uint16_t port);
        bool SendData(std::string_view data) const;
        void Close();

        [[nodiscard]] bool IsValid() const noexcept { return m_socket != INVALID_SOCKET; }
        [[nodiscard]] SOCKET GetNativeSocket() const noexcept { return m_socket; }
    };

    class NetworkManager {
    private:
        WsaInitScope m_wsaScope;
    public:
        NetworkManager() = default;
        ~NetworkManager() = default;

        [[nodiscard]] bool IsReady() const noexcept { return m_wsaScope.IsInitialized(); }
        [[nodiscard]] std::unique_ptr<SocketChannel> CreateTcpClient(std::string_view host, uint16_t port);
    };

} // namespace SysCore::Network

#endif // SYSCORE_NETWORK_MANAGER_H
