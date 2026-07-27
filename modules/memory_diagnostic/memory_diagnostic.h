#ifndef MEMORY_DIAGNOSTIC_MODULE_H
#define MEMORY_DIAGNOSTIC_MODULE_H

#include "core/imodule.h"
#include "core/resource_manager.h"
#include <windows.h>
#include <cstdint>
#include <vector>

struct MemoryUsageStats {
    size_t totalCommittedBytes{0};
    size_t totalReservedBytes{0};
    size_t totalPrivateBytes{0};
    size_t regionCount{0};
};

struct MemoryRegionInfo {
    uintptr_t baseAddress;
    size_t sizeBytes;
    DWORD type;
    DWORD protection;
};

class MemoryDiagnosticModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};
    DWORD m_targetProcessId{0};

public:
    MemoryDiagnosticModule() = default;
    ~MemoryDiagnosticModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "MemoryDiagnosticModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "2.0.0"; }

    // Module-specific diagnostic methods
    [[nodiscard]] MemoryUsageStats PerformMemoryScan(HANDLE hProcess) const noexcept;
    [[nodiscard]] std::vector<MemoryRegionInfo> CollectExecutableRegions(HANDLE hProcess) const noexcept;
};

#endif // MEMORY_DIAGNOSTIC_MODULE_H
