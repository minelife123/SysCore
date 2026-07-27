#include "pipe_diagnostic.h"
#include "core/logger.h"
#include "core/pipe_manager.h"
#include <thread>

bool PipeDiagnosticModule::Initialize() {
    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[PipeDiagnosticModule] Initializing Named Pipe diagnostic module...");
    m_initialized = true;
    return true;
}

bool PipeDiagnosticModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();

    SysCore::IPC::NamedPipeChannel server(L"SysCore_DiagPipe");
    if (server.CreateServer()) {
        logger.Info("[PipeDiagnosticModule] Named Pipe server Created -> '\\\\.\\pipe\\SysCore_DiagPipe'");
    } else {
        logger.Error("[PipeDiagnosticModule] Failed to create Named Pipe server");
        return false;
    }

    return true;
}

void PipeDiagnosticModule::Shutdown() {
    if (m_initialized) {
        SysCore::Logging::LoggerCore::Instance().Info("[PipeDiagnosticModule] Shutting down named pipe diagnostic module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new PipeDiagnosticModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
