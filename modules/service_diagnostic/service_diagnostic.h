#ifndef SERVICE_DIAGNOSTIC_MODULE_H
#define SERVICE_DIAGNOSTIC_MODULE_H

#include "core/imodule.h"
#include <cstdint>

struct ServiceAuditMetrics {
    size_t totalServicesCount{0};
    size_t runningServicesCount{0};
};

class ServiceDiagnosticModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};

public:
    ServiceDiagnosticModule() = default;
    ~ServiceDiagnosticModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "ServiceDiagnosticModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "8.0.0"; }

    [[nodiscard]] ServiceAuditMetrics AuditServices() const noexcept;
};

#endif // SERVICE_DIAGNOSTIC_MODULE_H
