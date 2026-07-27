#include "disk_diagnostic.h"
#include "core/logger.h"
#include <format>

bool DiskDiagnosticModule::Initialize() {
    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[DiskDiagnosticModule] Initializing disk storage diagnostic module...");
    m_initialized = true;
    return true;
}

DiskSpaceMetrics DiskDiagnosticModule::AuditDiskSpace() const noexcept {
    DiskSpaceMetrics metrics{};
    ULARGE_INTEGER freeBytesAvail{}, totalBytes{}, totalFreeBytes{};

    if (::GetDiskFreeSpaceExW(L"C:\\", &freeBytesAvail, &totalBytes, &totalFreeBytes)) {
        metrics.totalBytes = totalBytes.QuadPart;
        metrics.freeBytes = freeBytesAvail.QuadPart;
        if (metrics.totalBytes > 0) {
            metrics.freePercent = (static_cast<double>(metrics.freeBytes) / static_cast<double>(metrics.totalBytes)) * 100.0;
        }
    }

    return metrics;
}

bool DiskDiagnosticModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();
    DiskSpaceMetrics metrics = AuditDiskSpace();

    double totalGb = static_cast<double>(metrics.totalBytes) / (1024.0 * 1024.0 * 1024.0);
    double freeGb = static_cast<double>(metrics.freeBytes) / (1024.0 * 1024.0 * 1024.0);

    logger.LogFormat(SysCore::Logging::LogLevel::Info,
        "[DiskDiagnosticModule] Drive C:\\ Storage -> Free: {:.1f} GB / Total: {:.1f} GB ({:.1f}% free)",
        freeGb, totalGb, metrics.freePercent);

    return true;
}

void DiskDiagnosticModule::Shutdown() {
    if (m_initialized) {
        SysCore::Logging::LoggerCore::Instance().Info("[DiskDiagnosticModule] Shutting down disk diagnostic module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new DiskDiagnosticModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
