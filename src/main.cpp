#include "core/core.h"
#include <iostream>
#include <exception>
#include <filesystem>
#include <windows.h>

int main(int argc, char* argv[]) {
    try {
        SysCore::Core::ApplicationConfig config;
        config.appName = "SysCore Windows x64 Framework";
        config.logFilePath = "syscore_application.log";
        config.minLogLevel = SysCore::Logging::LogLevel::Debug;
        config.enableConsoleLogging = true;

        // Smart resolution of modules directory relative to executable and working directory
        std::filesystem::path exeDir;
        WCHAR exePathBuf[MAX_PATH]{0};
        if (::GetModuleFileNameW(NULL, exePathBuf, MAX_PATH) > 0) {
            exeDir = std::filesystem::path(exePathBuf).parent_path();
        }

        std::filesystem::path candidateDirs[] = {
            "build/bin/modules/Debug",
            "build/bin/modules/Release",
            exeDir / "../bin/modules/Debug",
            exeDir / "../bin/modules/Release",
            exeDir / "bin/modules/Debug",
            exeDir / "bin/modules/Release",
            "modules",
            exeDir / "modules"
        };

        std::filesystem::path selectedModulesDir = "build/bin/modules/Debug";
        for (const auto& dir : candidateDirs) {
            if (std::filesystem::exists(dir) && !std::filesystem::is_empty(dir)) {
                selectedModulesDir = dir;
                break;
            }
        }
        config.modulesDirectory = selectedModulesDir;

        SysCore::Core::ApplicationCore app;

        if (!app.Initialize(config)) {
            std::cerr << "Failed to initialize ApplicationCore!" << std::endl;
            return 1;
        }

        bool isCiMode = false;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--ci" || arg == "--non-interactive" || arg == "-y") {
                isCiMode = true;
            }
        }

        int exitCode = app.Run();
        app.Shutdown();

        std::cout << "\n=================================================" << std::endl;
        std::cout << "[SysCore Engine] Finished successfully." << std::endl;
        std::cout << "=================================================" << std::endl;

        if (!isCiMode && ::GetStdHandle(STD_INPUT_HANDLE) != NULL) {
            // Check if running interactively
            DWORD mode = 0;
            if (::GetConsoleMode(::GetStdHandle(STD_INPUT_HANDLE), &mode)) {
                std::cout << "Press Enter to exit..." << std::endl;
                std::cin.get();
            }
        }

        return exitCode;

    } catch (const std::exception& ex) {
        std::cerr << "Unhandled Exception in Main: " << ex.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Unknown Exception in Main!" << std::endl;
        return -1;
    }
}
