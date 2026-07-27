#include "system_metrics.h"
#include "core/logger.h"
#include "core/ipc_manager.h"
#include <iostream>
#include <format>

bool SystemMetricsModule::Initialize() {
    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[SystemMetricsModule] Initializing hardware telemetry module...");
    m_initialized = true;
    return true;
}

SystemHardwareMetrics SystemMetricsModule::CollectMetrics() const noexcept {
    SystemHardwareMetrics metrics{};

    MEMORYSTATUSEX statex{};
    statex.dwLength = sizeof(statex);
    if (::GlobalMemoryStatusEx(&statex)) {
        metrics.memoryLoadPercent = statex.dwMemoryLoad;
        metrics.totalPhysicalMemoryBytes = statex.ullTotalPhys;
        metrics.availablePhysicalMemoryBytes = statex.ullAvailPhys;
    }

    DWORD handleCount = 0;
    if (::GetProcessHandleCount(::GetCurrentProcess(), &handleCount)) {
        metrics.processHandleCount = handleCount;
    }

    return metrics;
}

bool SystemMetricsModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();
    SystemHardwareMetrics metrics = CollectMetrics();

    double totalMb = static_cast<double>(metrics.totalPhysicalMemoryBytes) / (1024.0 * 1024.0);
    double availMb = static_cast<double>(metrics.availablePhysicalMemoryBytes) / (1024.0 * 1024.0);

    logger.LogFormat(SysCore::Logging::LogLevel::Info,
        "[SystemMetricsModule] Telemetry Report -> RAM Load: {}% | Avail: {:.1f} MB / Total: {:.1f} MB | Handles: {}",
        metrics.memoryLoadPercent, availMb, totalMb, metrics.processHandleCount);

    return true;
}

void SystemMetricsModule::Shutdown() {
    if (m_initialized) {
        SysCore::Logging::LoggerCore::Instance().Info("[SystemMetricsModule] Shutting down metrics module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new SystemMetricsModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
