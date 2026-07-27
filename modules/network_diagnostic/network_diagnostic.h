#ifndef NETWORK_DIAGNOSTIC_MODULE_H
#define NETWORK_DIAGNOSTIC_MODULE_H

#include "core/imodule.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <cstdint>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

struct NetworkSocketMetrics {
    size_t activeTcpConnectionCount{0};
    uint32_t currentPid{0};
};

class NetworkDiagnosticModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};

public:
    NetworkDiagnosticModule() = default;
    ~NetworkDiagnosticModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "NetworkDiagnosticModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "5.0.0"; }

    [[nodiscard]] NetworkSocketMetrics AuditProcessSockets() const noexcept;
};

#endif // NETWORK_DIAGNOSTIC_MODULE_H
