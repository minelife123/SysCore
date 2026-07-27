#ifndef DISK_DIAGNOSTIC_MODULE_H
#define DISK_DIAGNOSTIC_MODULE_H

#include "core/imodule.h"
#include <windows.h>
#include <cstdint>

struct DiskSpaceMetrics {
    uint64_t totalBytes{0};
    uint64_t freeBytes{0};
    double freePercent{0.0};
};

class DiskDiagnosticModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};

public:
    DiskDiagnosticModule() = default;
    ~DiskDiagnosticModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "DiskDiagnosticModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "7.0.0"; }

    [[nodiscard]] DiskSpaceMetrics AuditDiskSpace() const noexcept;
};

#endif // DISK_DIAGNOSTIC_MODULE_H
