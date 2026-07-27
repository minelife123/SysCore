#include "core/network_monitor.h"
#include <psapi.h>
#include <sstream>
#include <vector>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace SysCore::Network {

    std::string NetworkMonitor::StateToString(DWORD state) noexcept
    {
        switch (state)
        {
        case MIB_TCP_STATE_CLOSED:     return "CLOSED";
        case MIB_TCP_STATE_LISTEN:     return "LISTENING";
        case MIB_TCP_STATE_SYN_SENT:  return "SYN_SENT";
        case MIB_TCP_STATE_SYN_RCVD:  return "SYN_RCVD";
        case MIB_TCP_STATE_ESTAB:     return "ESTABLISHED";
        case MIB_TCP_STATE_FIN_WAIT1: return "FIN_WAIT1";
        case MIB_TCP_STATE_FIN_WAIT2: return "FIN_WAIT2";
        case MIB_TCP_STATE_CLOSE_WAIT:return "CLOSE_WAIT";
        case MIB_TCP_STATE_CLOSING:   return "CLOSING";
        case MIB_TCP_STATE_LAST_ACK:  return "LAST_ACK";
        case MIB_TCP_STATE_TIME_WAIT: return "TIME_WAIT";
        case MIB_TCP_STATE_DELETE_TCB:return "DELETE_TCB";
        default:                      return "UNKNOWN";
        }
    }

    static std::string FormatIpAddress(DWORD dwAddr)
    {
        in_addr in;
        in.S_un.S_addr = dwAddr;
        char ipBuf[INET_ADDRSTRLEN] = { 0 };
        inet_ntop(AF_INET, &in, ipBuf, sizeof(ipBuf));
        return std::string(ipBuf);
    }

    static std::string GetProcessNameFromPid(DWORD pid)
    {
        if (pid == 0) return "System Idle";
        if (pid == 4) return "System";

        HANDLE hProc = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProc) return "Unknown";

        char exePath[MAX_PATH] = { 0 };
        DWORD size = MAX_PATH;
        if (::QueryFullProcessImageNameA(hProc, 0, exePath, &size))
        {
            ::CloseHandle(hProc);
            std::string fullPath(exePath);
            size_t pos = fullPath.find_last_of("\\/");
            if (pos != std::string::npos) return fullPath.substr(pos + 1);
            return fullPath;
        }
        ::CloseHandle(hProc);
        return "Unknown";
    }

    std::vector<NetworkSocketInfo> NetworkMonitor::GetActiveSockets() noexcept
    {
        std::vector<NetworkSocketInfo> sockets;

        // 1. Get Extended TCP Table
        DWORD tableSize = 0;
        ::GetExtendedTcpTable(nullptr, &tableSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
        if (tableSize > 0)
        {
            std::vector<unsigned char> buffer(tableSize);
            PMIB_TCPTABLE_OWNER_PID pTcpTable = reinterpret_cast<PMIB_TCPTABLE_OWNER_PID>(buffer.data());
            if (::GetExtendedTcpTable(pTcpTable, &tableSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR)
            {
                for (DWORD i = 0; i < pTcpTable->dwNumEntries; ++i)
                {
                    const auto& row = pTcpTable->table[i];
                    NetworkSocketInfo info{};
                    info.protocol = "TCP";
                    info.localAddress = FormatIpAddress(row.dwLocalAddr);
                    info.localPort = ntohs(static_cast<u_short>(row.dwLocalPort));
                    info.remoteAddress = FormatIpAddress(row.dwRemoteAddr);
                    info.remotePort = ntohs(static_cast<u_short>(row.dwRemotePort));
                    info.state = StateToString(row.dwState);
                    info.processId = row.dwOwningPid;
                    info.processName = GetProcessNameFromPid(row.dwOwningPid);
                    sockets.push_back(info);
                }
            }
        }

        // 2. Get Extended UDP Table
        tableSize = 0;
        ::GetExtendedUdpTable(nullptr, &tableSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0);
        if (tableSize > 0)
        {
            std::vector<unsigned char> buffer(tableSize);
            PMIB_UDPTABLE_OWNER_PID pUdpTable = reinterpret_cast<PMIB_UDPTABLE_OWNER_PID>(buffer.data());
            if (::GetExtendedUdpTable(pUdpTable, &tableSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR)
            {
                for (DWORD i = 0; i < pUdpTable->dwNumEntries; ++i)
                {
                    const auto& row = pUdpTable->table[i];
                    NetworkSocketInfo info{};
                    info.protocol = "UDP";
                    info.localAddress = FormatIpAddress(row.dwLocalAddr);
                    info.localPort = ntohs(static_cast<u_short>(row.dwLocalPort));
                    info.remoteAddress = "*";
                    info.remotePort = 0;
                    info.state = "UDP_ACTIVE";
                    info.processId = row.dwOwningPid;
                    info.processName = GetProcessNameFromPid(row.dwOwningPid);
                    sockets.push_back(info);
                }
            }
        }

        return sockets;
    }

} // namespace SysCore::Network
