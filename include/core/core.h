#ifndef SYSCORE_CORE_H
#define SYSCORE_CORE_H

#include "logger.h"
#include "resource_manager.h"
#include "module_manager.h"
#include "thread_manager.h"
#include "ipc_manager.h"
#include <string>
#include <filesystem>
#include <memory>

namespace SysCore::Core {

    struct ApplicationConfig {
        std::string appName{"ModularSystemFramework"};
        std::string logFilePath{"system_core.log"};
        Logging::LogLevel minLogLevel{Logging::LogLevel::Debug};
        bool enableConsoleLogging{true};
        std::filesystem::path modulesDirectory{"modules"};
        size_t workerThreadCount{0}; // 0 = default (hardware concurrency)
        std::wstring ipcChannelName{L"SysCore_SharedIPCChannel"};
    };

    class ApplicationCore {
    private:
        ApplicationConfig m_config;
        Modules::ModuleManager m_moduleManager;
        std::unique_ptr<Threading::ThreadManager> m_threadManager;
        std::unique_ptr<IPC::IpcManager> m_ipcManager;
        bool m_isInitialized{false};

    public:
        ApplicationCore() = default;
        ~ApplicationCore();

        ApplicationCore(const ApplicationCore&) = delete;
        ApplicationCore& operator=(const ApplicationCore&) = delete;

        // Lifecycle operations
        [[nodiscard]] bool Initialize(const ApplicationConfig& config = ApplicationConfig());
        [[nodiscard]] int Run();
        void Shutdown();

        // Subsystem accessors
        [[nodiscard]] Modules::ModuleManager& GetModuleManager() noexcept { return m_moduleManager; }
        [[nodiscard]] Threading::ThreadManager& GetThreadManager() { return *m_threadManager; }
        [[nodiscard]] IPC::IpcManager& GetIpcManager() { return *m_ipcManager; }
        [[nodiscard]] Logging::LoggerCore& GetLogger() noexcept { return Logging::LoggerCore::Instance(); }
        [[nodiscard]] bool IsInitialized() const noexcept { return m_isInitialized; }
    };

} // namespace SysCore::Core

#endif // SYSCORE_CORE_H
