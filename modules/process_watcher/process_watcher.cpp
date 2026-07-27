#include "process_watcher.h"
#include "core/logger.h"
#include "core/handle.h"
#include "core/ipc_manager.h"
#include <iostream>
#include <format>

bool ProcessWatcherModule::Initialize() {
    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[ProcessWatcherModule] Initializing process watcher module...");
    m_initialized = true;
    return true;
}

ProcessSnapshotMetrics ProcessWatcherModule::CountActiveProcesses() const noexcept {
    ProcessSnapshotMetrics metrics{};
    metrics.currentPid = ::GetCurrentProcessId();

    SysCore::Core::FileHandle hSnap(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!hSnap.IsValid()) {
        return metrics;
    }

    PROCESSENTRY32W pe32{};
    pe32.dwSize = sizeof(pe32);

    if (::Process32FirstW(hSnap.Get(), &pe32)) {
        do {
            metrics.activeProcessCount++;
        } while (::Process32NextW(hSnap.Get(), &pe32));
    }

    return metrics;
}

bool ProcessWatcherModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();
    ProcessSnapshotMetrics metrics = CountActiveProcesses();

    logger.LogFormat(SysCore::Logging::LogLevel::Info,
        "[ProcessWatcherModule] Active System Processes: {} | Host Process PID: {}",
        metrics.activeProcessCount, metrics.currentPid);

    return true;
}

void ProcessWatcherModule::Shutdown() {
    if (m_initialized) {
        SysCore::Logging::LoggerCore::Instance().Info("[ProcessWatcherModule] Shutting down process watcher module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new ProcessWatcherModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
