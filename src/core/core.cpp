#include "core/core.h"
#include "core/export_resolver.h"
#include "core/profiler.h"
#include <iostream>
#include <filesystem>

namespace SysCore::Core {

    ApplicationCore::~ApplicationCore() {
        Shutdown();
    }

    bool ApplicationCore::Initialize(const ApplicationConfig& config) {
        m_config = config;
        auto& logger = Logging::LoggerCore::Instance();

        if (!logger.Initialize(m_config.logFilePath, m_config.minLogLevel, m_config.enableConsoleLogging)) {
            std::cerr << "[ApplicationCore] Critical failure during logger initialization." << std::endl;
            return false;
        }

        logger.Info("=================================================");
        logger.Info(" Starting " + m_config.appName + " Core Subsystem");
        logger.Info("=================================================");

        // Architecture check
        bool is64Bit = Resources::ProcessManager::IsCurrentProcess64Bit();
        logger.Info("Platform Architecture: " + std::string(is64Bit ? "Windows x64" : "Windows x86"));

        if (!is64Bit) {
            logger.Warning("Application running under 32-bit emulation mode!");
        }

        // Auto-discover and load dynamic modules if module directory exists
        if (std::filesystem::exists(m_config.modulesDirectory)) {
            logger.Info("Discovering dynamic modules in: " + m_config.modulesDirectory.string());
            for (const auto& entry : std::filesystem::directory_iterator(m_config.modulesDirectory)) {
                if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                    (void)m_moduleManager.LoadModuleFromDll(entry.path());
                }
            }
        } else {
            logger.Info("Modules directory not found at '" + m_config.modulesDirectory.string() + "'. Skipping auto-discovery.");
        }

        // Initialize ThreadManager
        size_t threads = m_config.workerThreadCount == 0 ? std::thread::hardware_concurrency() : m_config.workerThreadCount;
        m_threadManager = std::make_unique<Threading::ThreadManager>(threads);

        // Initialize IPC Subsystem
        m_ipcManager = std::make_unique<IPC::IpcManager>(m_config.ipcChannelName);
        logger.Info("IPC Subsystem (SharedMemoryChannel) initialized.");

        m_isInitialized = true;
        logger.Info("ApplicationCore successfully initialized.");
        return true;
    }

    int ApplicationCore::Run() {
        auto& logger = Logging::LoggerCore::Instance();

        if (!m_isInitialized) {
            logger.Error("ApplicationCore::Run called before successful initialization.");
            return -1;
        }

        logger.Info("Executing ApplicationCore runtime loop...");

        // Demonstrate ThreadPool and Kernel Synchronization primitives
        {
            logger.Info("Demonstrating ThreadPool & Kernel Synchronization...");
            Threading::KernelEvent completionEvent(true, false);

            auto futureResult = m_threadManager->GetThreadPool().EnqueueTask([&completionEvent, &logger]() -> int {
                logger.Info("[Async Worker] Executing background task in ThreadPool...");
                ::Sleep(100); // Simulate workload
                completionEvent.Signal();
                return 42;
            });

            if (completionEvent.Wait(2000)) {
                int asyncVal = futureResult.get();
                logger.LogFormat(Logging::LogLevel::Info, "ThreadPool task completed with result: {}", asyncVal);
            } else {
                logger.Error("ThreadPool task timed out!");
            }
        }

        // Demonstrate SharedMemory Ring Buffer IPC Subsystem
        {
            logger.Info("Demonstrating SharedMemory IPC Ring Buffer Queue...");
            m_ipcManager->PushRingBufferMessage("Telemetry Frame #1: ApplicationCore Subsystem Active");
            m_ipcManager->PushRingBufferMessage("Telemetry Frame #2: Memory Manager Ready");
            m_ipcManager->PushRingBufferMessage("Telemetry Frame #3: ThreadPool Health Normal");

            IPC::SharedTelemetryHeader header{};
            while (m_ipcManager->PopRingBufferMessage(header)) {
                logger.LogFormat(Logging::LogLevel::Info,
                    "IPC RingBuffer Received [Seq: {} | Sender PID: {}]: {}",
                    header.sequenceNumber.load(), header.senderProcessId, header.messagePayload);
            }
        }

        // Demonstrate ExportResolver functionality
        {
            logger.Info("Demonstrating ExportResolver PE export parsing...");
            HMODULE hKernel32 = ::GetModuleHandleW(L"kernel32.dll");
            if (hKernel32) {
                uintptr_t pCreateFileW = Utils::ExportResolver::ResolveExportAddress(hKernel32, "CreateFileW");
                logger.LogFormat(Logging::LogLevel::Info, "ExportResolver: CreateFileW resolved to address 0x{:X}", pCreateFileW);
            }
        }

        // Demonstrate core RAII virtual memory management
        {
            logger.Info("Allocating RAII Virtual Memory block (64 KB)...");
            Resources::ScopedVirtualMem vmem(64 * 1024);
            if (vmem.IsValid()) {
                logger.Info("Virtual Memory allocated successfully at address: " +
                            std::to_string(reinterpret_cast<uintptr_t>(vmem.Get())));

                // Fill block with pattern
                std::memset(vmem.Get(), 0xAA, 1024);
                logger.Info("Virtual Memory block initialized with test pattern 0xAA.");
            }
            logger.Info("Exiting Virtual Memory scope - automatic RAII VirtualFree will execute.");
        }

        // Execute all loaded dynamic modules
        logger.Info("Executing dynamic modules...");
        bool result = m_moduleManager.ExecuteAllModules();

        if (result) {
            logger.Info("All modules executed successfully.");
        } else {
            logger.Warning("One or more modules failed during execution.");
        }

        // Demonstrate Module Hot-Reloading mechanism
        {
            logger.Info("Demonstrating Dynamic Module Hot-Reload...");
            m_moduleManager.ReloadModule("SampleMemoryModule");
            logger.Info("Checking for updated binaries on disk...");
            size_t reloaded = m_moduleManager.CheckAndHotReloadModules();
            logger.LogFormat(Logging::LogLevel::Info, "Hot-Reload Manager: {} modules updated on the fly.", reloaded);
        }

        return 0;
    }

    void ApplicationCore::Shutdown() {
        if (m_isInitialized) {
            auto& logger = Logging::LoggerCore::Instance();
            logger.Info("Shutting down ApplicationCore and releasing resources...");

            m_moduleManager.UnloadAllModules();
            if (m_threadManager) {
                m_threadManager->GetThreadPool().Shutdown();
                m_threadManager.reset();
            }
            if (m_ipcManager) {
                m_ipcManager.reset();
            }

            Profiling::PerformanceProfiler::Instance().LogReport();

            logger.Info("ApplicationCore shutdown complete.");
            logger.Shutdown();

            m_isInitialized = false;
        }
    }

} // namespace SysCore::Core
