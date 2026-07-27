#ifndef DRIVER_DIAGNOSTIC_MODULE_H
#define DRIVER_DIAGNOSTIC_MODULE_H

#include "core/imodule.h"
#include <cstdint>
#include <string>

struct DriverAuditMetrics {
    size_t loadedDriversCount{0};
    std::string ntoskrnlAddress;
};

class DriverDiagnosticModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};

public:
    DriverDiagnosticModule() = default;
    ~DriverDiagnosticModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "DriverDiagnosticModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "11.0.0"; }

    [[nodiscard]] DriverAuditMetrics AuditDrivers() const noexcept;
};

#endif // DRIVER_DIAGNOSTIC_MODULE_H
