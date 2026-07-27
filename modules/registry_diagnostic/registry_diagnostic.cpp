#include "registry_diagnostic.h"
#include "core/logger.h"
#include "core/registry_manager.h"

bool RegistryDiagnosticModule::Initialize() {
    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[RegistryDiagnosticModule] Initializing Windows Registry diagnostic module...");
    m_initialized = true;
    return true;
}

RegistryAuditMetrics RegistryDiagnosticModule::AuditRegistry() const noexcept {
    RegistryAuditMetrics metrics{};

    auto prodName = SysCore::Registry::RegistryManager::ReadString(
        SysCore::Registry::RegHive::LocalMachine,
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        "ProductName");

    auto buildNum = SysCore::Registry::RegistryManager::ReadString(
        SysCore::Registry::RegHive::LocalMachine,
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        "CurrentBuildNumber");

    if (prodName.has_value()) metrics.productName = prodName.value();
    if (buildNum.has_value()) metrics.currentBuild = buildNum.value();

    auto autoruns = SysCore::Registry::RegistryManager::EnumSubkeyValues(
        SysCore::Registry::RegHive::LocalMachine,
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");

    metrics.autorunEntriesCount = autoruns.size();
    return metrics;
}

bool RegistryDiagnosticModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();
    RegistryAuditMetrics metrics = AuditRegistry();

    logger.LogFormat(SysCore::Logging::LogLevel::Info,
        "[RegistryDiagnosticModule] OS: '{}' (Build {}) | System HKLM Autorun Entries: {}",
        metrics.productName, metrics.currentBuild, metrics.autorunEntriesCount);

    return true;
}

void RegistryDiagnosticModule::Shutdown() {
    if (m_initialized) {
        SysCore::Logging::LoggerCore::Instance().Info("[RegistryDiagnosticModule] Shutting down registry diagnostic module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new RegistryDiagnosticModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
