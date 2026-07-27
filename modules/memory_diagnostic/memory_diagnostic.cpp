#include "memory_diagnostic.h"
#include "core/logger.h"
#include "core/ipc_manager.h"
#include <iostream>
#include <format>

bool MemoryDiagnosticModule::Initialize() {
    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[MemoryDiagnosticModule] Initializing executive memory diagnostic module...");
    
    m_targetProcessId = ::GetCurrentProcessId();
    m_initialized = true;
    return true;
}

MemoryUsageStats MemoryDiagnosticModule::PerformMemoryScan(HANDLE hProcess) const noexcept {
    MemoryUsageStats stats{};
    if (!hProcess) return stats;

    MEMORY_BASIC_INFORMATION mbi{};
    LPCVOID address = nullptr;

    while (::VirtualQueryEx(hProcess, address, &mbi, sizeof(mbi)) == sizeof(mbi)) {
        stats.regionCount++;

        if (mbi.State == MEM_COMMIT) {
            stats.totalCommittedBytes += mbi.RegionSize;
            if (mbi.Type == MEM_PRIVATE) {
                stats.totalPrivateBytes += mbi.RegionSize;
            }
        } else if (mbi.State == MEM_RESERVE) {
            stats.totalReservedBytes += mbi.RegionSize;
        }

        address = static_cast<const char*>(mbi.BaseAddress) + mbi.RegionSize;
    }

    return stats;
}

std::vector<MemoryRegionInfo> MemoryDiagnosticModule::CollectExecutableRegions(HANDLE hProcess) const noexcept {
    std::vector<MemoryRegionInfo> regions;
    if (!hProcess) return regions;

    MEMORY_BASIC_INFORMATION mbi{};
    LPCVOID address = nullptr;

    while (::VirtualQueryEx(hProcess, address, &mbi, sizeof(mbi)) == sizeof(mbi)) {
        bool isExecutable = (mbi.Protect & PAGE_EXECUTE) ||
                            (mbi.Protect & PAGE_EXECUTE_READ) ||
                            (mbi.Protect & PAGE_EXECUTE_READWRITE) ||
                            (mbi.Protect & PAGE_EXECUTE_WRITECOPY);

        if (mbi.State == MEM_COMMIT && isExecutable) {
            regions.push_back({
                reinterpret_cast<uintptr_t>(mbi.BaseAddress),
                mbi.RegionSize,
                mbi.Type,
                mbi.Protect
            });
        }
        address = static_cast<const char*>(mbi.BaseAddress) + mbi.RegionSize;
    }
    return regions;
}

bool MemoryDiagnosticModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info(std::format("[MemoryDiagnosticModule] Performing memory analysis for PID: {}...", m_targetProcessId));

    // Open process using core RAII handle
    auto procHandle = SysCore::Resources::ProcessManager::OpenProcess(m_targetProcessId, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ);
    if (!procHandle.IsValid()) {
        logger.Error("[MemoryDiagnosticModule] Failed to acquire handle for target process.");
        return false;
    }

    MemoryUsageStats stats = PerformMemoryScan(procHandle.Get());
    auto execRegions = CollectExecutableRegions(procHandle.Get());

    double committedMb = static_cast<double>(stats.totalCommittedBytes) / (1024.0 * 1024.0);
    double reservedMb = static_cast<double>(stats.totalReservedBytes) / (1024.0 * 1024.0);
    double privateMb = static_cast<double>(stats.totalPrivateBytes) / (1024.0 * 1024.0);

    logger.LogFormat(SysCore::Logging::LogLevel::Info,
        "[MemoryDiagnosticModule] Diagnostic Scan Complete -> Total Regions: {} | Executable Regions: {} | Committed: {:.2f} MB | Reserved: {:.2f} MB | Private: {:.2f} MB",
        stats.regionCount, execRegions.size(), committedMb, reservedMb, privateMb);

    // Audit for W^X violations (PAGE_EXECUTE_READWRITE)
    size_t wxViolations = 0;
    for (const auto& reg : execRegions) {
        if (reg.protection & PAGE_EXECUTE_READWRITE) {
            wxViolations++;
        }
    }

    if (wxViolations > 0) {
        logger.LogFormat(SysCore::Logging::LogLevel::Warning,
            "[MemoryDiagnosticModule] W^X Security Audit Warning: Found {} regions with PAGE_EXECUTE_READWRITE protection!", wxViolations);
    } else {
        logger.Info("[MemoryDiagnosticModule] W^X Security Audit Passed: No PAGE_EXECUTE_READWRITE regions detected.");
    }

    return true;
}

void MemoryDiagnosticModule::Shutdown() {
    if (m_initialized) {
        SysCore::Logging::LoggerCore::Instance().Info("[MemoryDiagnosticModule] Shutting down diagnostic module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new MemoryDiagnosticModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
