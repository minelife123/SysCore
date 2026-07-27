#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "core/network_manager.h"
#include "core/http_server.h"
#include "core/async_file.h"
#include "core/concepts.h"
#include "core/handle.h"
#include "core/logger.h"
#include "core/resource_manager.h"
#include "core/export_resolver.h"
#include "core/thread_manager.h"
#include "core/ipc_manager.h"
#include "core/config_manager.h"
#include "core/event_bus.h"
#include "core/profiler.h"
#include "core/module_verifier.h"
#include "core/hot_patch_manager.h"
#include "core/hardware_info.h"
#include "core/crypto_hash.h"
#include "core/service_manager.h"
#include "core/benchmark.h"
#include "core/registry_manager.h"
#include "core/pipe_manager.h"
#include "core/driver_info.h"
#include "core/token_manager.h"
#include "core/file_watcher.h"
#include <iostream>
#include <string>
#include <cassert>

namespace SysCore::Tests {

    static int g_passCount = 0;
    static int g_failCount = 0;

    #define ASSERT_TEST(condition, message) \
        if (condition) { \
            std::cout << "[PASS] " << message << std::endl; \
            g_passCount++; \
        } else { \
            std::cerr << "[FAIL] " << message << " (Line " << __LINE__ << ")" << std::endl; \
            g_failCount++; \
        }

    void Test_RAII_Handles() {
        std::cout << "\n--- Running Test_RAII_Handles ---" << std::endl;
        Core::ProcessHandle handle(::GetCurrentProcess());
        ASSERT_TEST(handle.IsValid(), "ProcessHandle should be valid for GetCurrentProcess()");

        Core::ProcessHandle movedHandle = std::move(handle);
        ASSERT_TEST(!handle.IsValid(), "Moved-from ProcessHandle should be invalid");
        ASSERT_TEST(movedHandle.IsValid(), "Moved-to ProcessHandle should be valid");
    }

    void Test_Virtual_Memory() {
        std::cout << "\n--- Running Test_Virtual_Memory ---" << std::endl;
        Resources::ScopedVirtualMem vmem(64 * 1024);
        ASSERT_TEST(vmem.IsValid(), "ScopedVirtualMem should allocate 64KB block");
        ASSERT_TEST(vmem.GetSize() == 64 * 1024, "ScopedVirtualMem size should match requested 64KB");

        if (vmem.IsValid()) {
            char* ptr = vmem.As<char>();
            ptr[0] = 'S';
            ptr[1] = 'Y';
            ptr[2] = 'S';
            ASSERT_TEST(ptr[0] == 'S' && ptr[1] == 'Y' && ptr[2] == 'S', "Virtual memory block write/read pattern check");
        }
    }

    void Test_PE_ExportResolver() {
        std::cout << "\n--- Running Test_PE_ExportResolver ---" << std::endl;
        HMODULE hKernel32 = ::GetModuleHandleW(L"kernel32.dll");
        ASSERT_TEST(hKernel32 != nullptr, "kernel32.dll handle should be retrieved");

        if (hKernel32) {
            uintptr_t addr = Utils::ExportResolver::ResolveExportAddress(hKernel32, "CreateFileW");
            ASSERT_TEST(addr != 0, "ExportResolver should resolve CreateFileW address in kernel32.dll");
        }
    }

    void Test_IPC_RingBuffer() {
        std::cout << "\n--- Running Test_IPC_RingBuffer ---" << std::endl;
        IPC::IpcManager ipc(L"SysCore_UnitTestIPCChannel");
        
        bool push1 = ipc.PushRingBufferMessage("Test Msg #1");
        bool push2 = ipc.PushRingBufferMessage("Test Msg #2");
        bool push3 = ipc.PushRingBufferMessage("Test Msg #3");
        ASSERT_TEST(push1 && push2 && push3, "IPC RingBuffer should accept 3 consecutive messages");

        IPC::SharedTelemetryHeader msg1{}, msg2{}, msg3{};
        bool pop1 = ipc.PopRingBufferMessage(msg1);
        bool pop2 = ipc.PopRingBufferMessage(msg2);
        bool pop3 = ipc.PopRingBufferMessage(msg3);

        ASSERT_TEST(pop1 && std::string(msg1.messagePayload) == "Test Msg #1", "IPC RingBuffer FIFO Pop #1 payload check");
        ASSERT_TEST(pop2 && std::string(msg2.messagePayload) == "Test Msg #2", "IPC RingBuffer FIFO Pop #2 payload check");
        ASSERT_TEST(pop3 && std::string(msg3.messagePayload) == "Test Msg #3", "IPC RingBuffer FIFO Pop #3 payload check");

        IPC::SharedTelemetryHeader msgEmpty{};
        ASSERT_TEST(!ipc.PopRingBufferMessage(msgEmpty), "IPC RingBuffer Pop on empty buffer should return false");
    }

    void Test_ThreadPool_Async() {
        std::cout << "\n--- Running Test_ThreadPool_Async ---" << std::endl;
        Threading::ThreadPool pool(4);
        ASSERT_TEST(pool.GetWorkerCount() == 4, "ThreadPool should initialize 4 worker threads");

        auto f1 = pool.EnqueueTask([]() { return 10; });
        auto f2 = pool.EnqueueTask([]() { return 20; });
        auto f3 = pool.EnqueueTask([]() { return 30; });

        ASSERT_TEST(f1.get() == 10, "ThreadPool async task #1 result check");
        ASSERT_TEST(f2.get() == 20, "ThreadPool async task #2 result check");
        ASSERT_TEST(f3.get() == 30, "ThreadPool async task #3 result check");
    }

    void Test_ConfigManager() {
        std::cout << "\n--- Running Test_ConfigManager ---" << std::endl;
        Config::ConfigManager config;
        config.SetString("app.title", "SysCore Unit Test App");
        config.SetInt("app.threads", 8);
        config.SetBool("app.debug", true);

        ASSERT_TEST(config.GetString("app.title") == "SysCore Unit Test App", "ConfigManager GetString check");
        ASSERT_TEST(config.GetInt("app.threads") == 8, "ConfigManager GetInt check");
        ASSERT_TEST(config.GetBool("app.debug") == true, "ConfigManager GetBool check");
        ASSERT_TEST(config.GetInt("missing.key", 100) == 100, "ConfigManager default fallback check");
    }

    struct CustomTestEvent {
        std::string payload;
    };

    void Test_EventBus() {
        std::cout << "\n--- Running Test_EventBus ---" << std::endl;
        Events::EventBus eventBus;
        std::string receivedData = "";

        eventBus.Subscribe<CustomTestEvent>([&receivedData](const CustomTestEvent& evt) {
            receivedData = evt.payload;
        });

        eventBus.Publish<CustomTestEvent>(CustomTestEvent{.payload = "EventBus Broadcast Data"});
        ASSERT_TEST(receivedData == "EventBus Broadcast Data", "EventBus publish/subscribe delivery check");
    }

    void Test_Profiler() {
        std::cout << "\n--- Running Test_Profiler ---" << std::endl;
        {
            SYSCORE_PROFILE_SCOPE("UnitTestTimerScope");
            ::Sleep(10);
        }

        auto results = Profiling::PerformanceProfiler::Instance().GetResults();
        bool foundScope = false;
        for (const auto& res : results) {
            if (res.name == "UnitTestTimerScope") {
                foundScope = (res.callCount == 1 && res.totalDurationMs >= 5.0);
                break;
            }
        }
        ASSERT_TEST(foundScope, "PerformanceProfiler scope timer recording check");
    }

    void Test_ModuleVerifier() {
        std::cout << "\n--- Running Test_ModuleVerifier ---" << std::endl;
        auto result = Modules::ModuleVerifier::VerifyModuleDll("non_existent.dll");
        ASSERT_TEST(!result.isValid, "ModuleVerifier should fail for non-existent DLL");
    }

    void Test_NetworkManager() {
        std::cout << "\n--- Running Test_NetworkManager ---" << std::endl;
        Network::NetworkManager netMgr;
        ASSERT_TEST(netMgr.IsReady(), "NetworkManager WinSock2 initialization check");
    }

    __declspec(noinline) static int TargetHookFunction(int a, int b) {
        return a + b;
    }

    __declspec(noinline) static int DetourHookFunction(int a, int b) {
        return (a + b) * 10;
    }

    void Test_HotPatchManager() {
        std::cout << "\n--- Running Test_HotPatchManager ---" << std::endl;
        auto& patcher = Instrumentation::HotPatchManager::Instance();
        void* pTarget = reinterpret_cast<void*>(&TargetHookFunction);
        void* pDetour = reinterpret_cast<void*>(&DetourHookFunction);

        volatile int x1 = 2, y1 = 3;
        bool installed = patcher.InstallHook(pTarget, pDetour);
        ASSERT_TEST(installed && patcher.IsHooked(pTarget), "HotPatchManager hook installation check");

        int hookedResult = TargetHookFunction(x1, y1);
        ASSERT_TEST(hookedResult == 50, "HotPatchManager trampoline execution detour check (expected 50)");

        bool removed = patcher.RemoveHook(pTarget);
        ASSERT_TEST(removed && !patcher.IsHooked(pTarget), "HotPatchManager hook removal check");

        volatile int x2 = 2, y2 = 3;
        int originalResult = TargetHookFunction(x2, y2);
        ASSERT_TEST(originalResult == 5, "HotPatchManager restored function execution check (expected 5)");
    }

    void Test_HardwareInfo() {
        std::cout << "\n--- Running Test_HardwareInfo ---" << std::endl;
        auto cpu = Hardware::HardwareInfo::GetCpuTopology();
        auto os = Hardware::HardwareInfo::GetOsVersion();
        ASSERT_TEST(cpu.logicalCoreCount > 0, "HardwareInfo CPU logical core count check");
        ASSERT_TEST(!os.osName.empty(), "HardwareInfo OS version string check");
    }

    void Test_CryptoHash() {
        std::cout << "\n--- Running Test_CryptoHash ---" << std::endl;
        std::string input = "SysCore Engine SHA-256 Checksum Test";
        std::string hash = Crypto::CryptoHashManager::ComputeSha256(input);
        ASSERT_TEST(!hash.empty() && hash.size() == 64, "CryptoHashManager SHA-256 64-char hex digest length check");
    }

    void Test_HttpServer() {
        std::cout << "\n--- Running Test_HttpServer ---" << std::endl;
        Network::TelemetryHttpServer server;
        bool started = server.Start(18080, [](const std::string&, const std::string&) {
            return "{\"status\":\"ok\"}";
        });
        ASSERT_TEST(started && server.IsRunning(), "TelemetryHttpServer background thread REST server start check");
        server.Stop();
        ASSERT_TEST(!server.IsRunning(), "TelemetryHttpServer graceful shutdown check");
    }

    void Test_AsyncFile() {
        std::cout << "\n--- Running Test_AsyncFile ---" << std::endl;
        std::string testPath = "syscore_async_test.tmp";
        std::string testContent = "SysCore Overlapped Async File I/O Content";

        auto futWrite = IO::AsyncFileEngine::WriteFileAsync(testPath, testContent);
        ASSERT_TEST(futWrite.get(), "AsyncFileEngine Overlapped write check");

        auto futRead = IO::AsyncFileEngine::ReadFileAsync(testPath);
        ASSERT_TEST(futRead.get() == testContent, "AsyncFileEngine Overlapped read check");

        std::filesystem::remove(testPath);
    }

    void Test_ServiceManager() {
        std::cout << "\n--- Running Test_ServiceManager ---" << std::endl;
        auto services = Services::ServiceManager::EnumActiveServices();
        ASSERT_TEST(!services.empty(), "ServiceManager SCM active services enumeration check");
    }

    void Test_BenchmarkEngine() {
        std::cout << "\n--- Running Test_BenchmarkEngine ---" << std::endl;
        int counter = 0;
        auto bench = Profiling::BenchmarkEngine::RunBenchmark("TestCounterIncrement", 1000, [&counter]() {
            counter++;
        });
        ASSERT_TEST(bench.iterations == 1000 && counter == 1001, "BenchmarkEngine precision micro-benchmark check");
    }

    void Test_RegistryManager() {
        std::cout << "\n--- Running Test_RegistryManager ---" << std::endl;
        auto buildNum = Registry::RegistryManager::ReadString(
            Registry::RegHive::LocalMachine,
            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
            "CurrentBuildNumber");
        ASSERT_TEST(buildNum.has_value() && !buildNum.value().empty(), "RegistryManager HKLM CurrentBuildNumber read check");
    }

    void Test_PipeManager() {
        std::cout << "\n--- Running Test_PipeManager ---" << std::endl;
        IPC::NamedPipeChannel pipeServer(L"SysCore_UnitTestPipe");
        ASSERT_TEST(pipeServer.CreateServer() && pipeServer.IsValid(), "NamedPipeChannel server creation check");
        pipeServer.Close();
    }

    void Test_DriverInfo() {
        std::cout << "\n--- Running Test_DriverInfo ---" << std::endl;
        auto drivers = Kernel::DriverInfo::EnumLoadedDrivers();
        ASSERT_TEST(!drivers.empty(), "DriverInfo PSAPI loaded device drivers enumeration check");
    }

    void Test_TokenManager() {
        std::cout << "\n--- Running Test_TokenManager ---" << std::endl;
        auto secCtx = Security::TokenManager::GetCurrentProcessSecurityContext();
        ASSERT_TEST(!secCtx.integrityLevelName.empty(), "TokenManager security context & integrity level check");
    }

    void Test_FileWatcher() {
        std::cout << "\n--- Running Test_FileWatcher ---" << std::endl;
        IO::DirectoryWatcher watcher("syscore_test_watch_dir");
        bool started = watcher.Start([](const IO::FileChangeEvent&) {});
        ASSERT_TEST(started && watcher.IsRunning(), "DirectoryWatcher Start and IsRunning check");
        watcher.Stop();
        ASSERT_TEST(!watcher.IsRunning(), "DirectoryWatcher Stop check");
        std::filesystem::remove_all("syscore_test_watch_dir");
    }

} // namespace SysCore::Tests

int main() {
    std::cout << "=================================================" << std::endl;
    std::cout << "      SysCore C++20 Automated Unit Test Suite    " << std::endl;
    std::cout << "=================================================" << std::endl;

    SysCore::Tests::Test_RAII_Handles();
    SysCore::Tests::Test_Virtual_Memory();
    SysCore::Tests::Test_PE_ExportResolver();
    SysCore::Tests::Test_IPC_RingBuffer();
    SysCore::Tests::Test_ThreadPool_Async();
    SysCore::Tests::Test_ConfigManager();
    SysCore::Tests::Test_EventBus();
    SysCore::Tests::Test_Profiler();
    SysCore::Tests::Test_ModuleVerifier();
    SysCore::Tests::Test_NetworkManager();
    SysCore::Tests::Test_HotPatchManager();
    SysCore::Tests::Test_HardwareInfo();
    SysCore::Tests::Test_CryptoHash();
    SysCore::Tests::Test_HttpServer();
    SysCore::Tests::Test_AsyncFile();
    SysCore::Tests::Test_ServiceManager();
    SysCore::Tests::Test_BenchmarkEngine();
    SysCore::Tests::Test_RegistryManager();
    SysCore::Tests::Test_PipeManager();
    SysCore::Tests::Test_DriverInfo();
    SysCore::Tests::Test_TokenManager();
    SysCore::Tests::Test_FileWatcher();

    std::cout << "\n=================================================" << std::endl;
    std::cout << " Test Summary: " << SysCore::Tests::g_passCount << " Passed | "
              << SysCore::Tests::g_failCount << " Failed" << std::endl;
    std::cout << "=================================================" << std::endl;

    return (SysCore::Tests::g_failCount == 0) ? 0 : 1;
}
