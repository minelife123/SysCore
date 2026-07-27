#include "core/network_manager.h"
#include "core/logger.h"

namespace SysCore::Network {

    bool SocketChannel::Create(int af, int type, int protocol) {
        Close();
        m_socket = ::WSASocketW(af, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
        return IsValid();
    }

    bool SocketChannel::Connect(std::string_view host, uint16_t port) {
        if (!IsValid() && !Create()) {
            return false;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = ::htons(port);
        ::inet_pton(AF_INET, std::string(host).c_str(), &serverAddr.sin_addr);

        int result = ::connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
        if (result == SOCKET_ERROR) {
            Logging::LoggerCore::Instance().Warning("NetworkManager: Failed to connect socket to " + std::string(host) + ":" + std::to_string(port));
            Close();
            return false;
        }

        return true;
    }

    bool SocketChannel::SendData(std::string_view data) const {
        if (!IsValid()) return false;
        int sent = ::send(m_socket, data.data(), static_cast<int>(data.size()), 0);
        return sent != SOCKET_ERROR;
    }

    void SocketChannel::Close() {
        if (m_socket != INVALID_SOCKET) {
            ::closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }
    }

    std::unique_ptr<SocketChannel> NetworkManager::CreateTcpClient(std::string_view host, uint16_t port) {
        auto client = std::make_unique<SocketChannel>();
        if (client->Connect(host, port)) {
            return client;
        }
        return nullptr;
    }

} // namespace SysCore::Network
