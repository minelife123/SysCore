#ifndef SYSCORE_NETWORK_MONITOR_H
#define SYSCORE_NETWORK_MONITOR_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <string>
#include <vector>
#include <cstdint>

namespace SysCore::Network {

    struct NetworkSocketInfo {
        std::string protocol;       // "TCP" or "UDP"
        std::string localAddress;
        uint16_t localPort{0};
        std::string remoteAddress;
        uint16_t remotePort{0};
        std::string state;          // "ESTABLISHED", "LISTENING", "TIME_WAIT", etc.
        DWORD processId{0};
        std::string processName;
    };

    class NetworkMonitor {
    public:
        [[nodiscard]] static std::vector<NetworkSocketInfo> GetActiveSockets() noexcept;
        [[nodiscard]] static std::string StateToString(DWORD state) noexcept;
    };

} // namespace SysCore::Network

#endif // SYSCORE_NETWORK_MONITOR_H
