#include "driver_diagnostic.h"
#include "core/logger.h"
#include "core/driver_info.h"
#include <format>

bool DriverDiagnosticModule::Initialize() {
    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[DriverDiagnosticModule] Initializing Kernel Driver diagnostic module...");
    m_initialized = true;
    return true;
}

DriverAuditMetrics DriverDiagnosticModule::AuditDrivers() const noexcept {
    DriverAuditMetrics metrics{};
    auto drivers = SysCore::Kernel::DriverInfo::EnumLoadedDrivers();
    metrics.loadedDriversCount = drivers.size();

    for (const auto& drv : drivers) {
        if (drv.baseName == "ntoskrnl.exe" || drv.baseName == "ntkrnlmp.exe") {
            metrics.ntoskrnlAddress = std::format("0x{:X}", drv.loadAddress);
            break;
        }
    }

    return metrics;
}

bool DriverDiagnosticModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();
    DriverAuditMetrics metrics = AuditDrivers();

    logger.LogFormat(SysCore::Logging::LogLevel::Info,
        "[DriverDiagnosticModule] Kernel Audit -> Loaded Drivers: {} | ntoskrnl Base: {}",
        metrics.loadedDriversCount, metrics.ntoskrnlAddress.empty() ? "N/A" : metrics.ntoskrnlAddress);

    return true;
}

void DriverDiagnosticModule::Shutdown() {
    if (m_initialized) {
        SysCore::Logging::LoggerCore::Instance().Info("[DriverDiagnosticModule] Shutting down driver diagnostic module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new DriverDiagnosticModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
