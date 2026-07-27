#include "service_diagnostic.h"
#include "core/logger.h"
#include "core/service_manager.h"

bool ServiceDiagnosticModule::Initialize() {
    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[ServiceDiagnosticModule] Initializing Windows service diagnostic module...");
    m_initialized = true;
    return true;
}

ServiceAuditMetrics ServiceDiagnosticModule::AuditServices() const noexcept {
    ServiceAuditMetrics metrics{};
    auto services = SysCore::Services::ServiceManager::EnumActiveServices();
    metrics.totalServicesCount = services.size();

    for (const auto& svc : services) {
        if (svc.isRunning) {
            metrics.runningServicesCount++;
        }
    }

    return metrics;
}

bool ServiceDiagnosticModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();
    ServiceAuditMetrics metrics = AuditServices();

    logger.LogFormat(SysCore::Logging::LogLevel::Info,
        "[ServiceDiagnosticModule] Windows Services Audit -> Running: {} / Total Discovered: {}",
        metrics.runningServicesCount, metrics.totalServicesCount);

    return true;
}

void ServiceDiagnosticModule::Shutdown() {
    if (m_initialized) {
        SysCore::Logging::LoggerCore::Instance().Info("[ServiceDiagnosticModule] Shutting down service diagnostic module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new ServiceDiagnosticModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
