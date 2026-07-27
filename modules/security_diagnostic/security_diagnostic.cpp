#include "security_diagnostic.h"
#include "core/logger.h"
#include "core/token_manager.h"

bool SecurityDiagnosticModule::Initialize() {
    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[SecurityDiagnosticModule] Initializing process token security diagnostic module...");
    m_initialized = true;
    return true;
}

bool SecurityDiagnosticModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();
    auto secCtx = SysCore::Security::TokenManager::GetCurrentProcessSecurityContext();

    logger.LogFormat(SysCore::Logging::LogLevel::Info,
        "[SecurityDiagnosticModule] Process Security Context -> Elevated: {} | Integrity Level: {} | Privileges Discovered: {}",
        secCtx.isElevated ? "YES (Admin)" : "NO (Standard User)",
        secCtx.integrityLevelName.empty() ? "Standard" : secCtx.integrityLevelName,
        secCtx.privileges.size());

    return true;
}

void SecurityDiagnosticModule::Shutdown() {
    if (m_initialized) {
        SysCore::Logging::LoggerCore::Instance().Info("[SecurityDiagnosticModule] Shutting down security diagnostic module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new SecurityDiagnosticModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
