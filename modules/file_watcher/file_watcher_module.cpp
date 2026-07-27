#include "file_watcher_module.h"
#include "core/logger.h"

bool FileWatcherModule::Initialize() {
    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[FileWatcherModule] Initializing directory watcher module...");

    m_watcher = std::make_unique<SysCore::IO::DirectoryWatcher>("syscore_watch_temp");
    bool started = m_watcher->Start([](const SysCore::IO::FileChangeEvent& evt) {
        SysCore::Logging::LoggerCore::Instance().LogFormat(
            SysCore::Logging::LogLevel::Info,
            "[FileWatcherModule] File System Event -> Path: {} | Action: {}",
            evt.path.string(), static_cast<int>(evt.action));
    });

    m_initialized = started;
    return started;
}

bool FileWatcherModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[FileWatcherModule] Directory Watcher Subsystem Active -> Watching 'syscore_watch_temp'");
    return true;
}

void FileWatcherModule::Shutdown() {
    if (m_initialized) {
        if (m_watcher) m_watcher->Stop();
        SysCore::Logging::LoggerCore::Instance().Info("[FileWatcherModule] Shutting down directory watcher module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new FileWatcherModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
