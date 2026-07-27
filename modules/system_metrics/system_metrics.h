#ifndef SYSTEM_METRICS_MODULE_H
#define SYSTEM_METRICS_MODULE_H

#include "core/imodule.h"
#include <windows.h>
#include <cstdint>

struct SystemHardwareMetrics {
    DWORD memoryLoadPercent{0};
    uint64_t totalPhysicalMemoryBytes{0};
    uint64_t availablePhysicalMemoryBytes{0};
    DWORD processHandleCount{0};
};

class SystemMetricsModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};

public:
    SystemMetricsModule() = default;
    ~SystemMetricsModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "SystemMetricsModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "3.0.0"; }

    [[nodiscard]] SystemHardwareMetrics CollectMetrics() const noexcept;
};

#endif // SYSTEM_METRICS_MODULE_H
