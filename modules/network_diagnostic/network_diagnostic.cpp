#include "network_diagnostic.h"
#include "core/logger.h"
#include <vector>
#include <iostream>

bool NetworkDiagnosticModule::Initialize() {
    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[NetworkDiagnosticModule] Initializing network socket diagnostic module...");
    m_initialized = true;
    return true;
}

NetworkSocketMetrics NetworkDiagnosticModule::AuditProcessSockets() const noexcept {
    NetworkSocketMetrics metrics{};
    metrics.currentPid = ::GetCurrentProcessId();

    DWORD size = 0;
    if (::GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == ERROR_INSUFFICIENT_BUFFER) {
        std::vector<BYTE> buffer(size);
        if (::GetExtendedTcpTable(buffer.data(), &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
            auto tcpTable = reinterpret_cast<MIB_TCPTABLE_OWNER_PID*>(buffer.data());
            for (DWORD i = 0; i < tcpTable->dwNumEntries; ++i) {
                if (tcpTable->table[i].dwOwningPid == metrics.currentPid) {
                    metrics.activeTcpConnectionCount++;
                }
            }
        }
    }

    return metrics;
}

bool NetworkDiagnosticModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();
    NetworkSocketMetrics metrics = AuditProcessSockets();

    logger.LogFormat(SysCore::Logging::LogLevel::Info,
        "[NetworkDiagnosticModule] Active Process Sockets -> TCP Connections: {} (PID: {})",
        metrics.activeTcpConnectionCount, metrics.currentPid);

    return true;
}

void NetworkDiagnosticModule::Shutdown() {
    if (m_initialized) {
        SysCore::Logging::LoggerCore::Instance().Info("[NetworkDiagnosticModule] Shutting down network diagnostic module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new NetworkDiagnosticModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
