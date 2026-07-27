#include "core/module_manager.h"
#include "core/module_verifier.h"
#include "core/profiler.h"
#include "core/logger.h"
#include <algorithm>

namespace SysCore::Modules {

    ModuleManager::~ModuleManager() {
        UnloadAllModules();
    }

    bool ModuleManager::LoadModuleFromDll(const std::filesystem::path& dllPath) {
        SYSCORE_PROFILE_FUNCTION();
        auto& logger = Logging::LoggerCore::Instance();

        // 1. Verify module integrity before executing DllMain
        auto verifyResult = ModuleVerifier::VerifyModuleDll(dllPath);
        if (!verifyResult.isValid) {
            logger.Error("ModuleManager: Verification failed for " + dllPath.string() + ": " + verifyResult.errorMessage);
            return false;
        }

        HMODULE hMod = ::LoadLibraryW(dllPath.c_str());
        if (!hMod) {
            logger.Error("ModuleManager: Failed to load library " + dllPath.string() +
                         " Error code: " + std::to_string(::GetLastError()));
            return false;
        }

        LibraryHandle libHandle(hMod);

        auto createFn = libHandle.GetFunction<Interfaces::CreateModuleFn>("CreateModule");
        auto destroyFn = libHandle.GetFunction<Interfaces::DestroyModuleFn>("DestroyModule");

        if (!createFn || !destroyFn) {
            logger.Error("ModuleManager: Required export functions (CreateModule/DestroyModule) missing in " + dllPath.string());
            return false;
        }

        Interfaces::IModule* rawModule = createFn();
        if (!rawModule) {
            logger.Error("ModuleManager: CreateModule returned nullptr in " + dllPath.string());
            return false;
        }

        std::string modName = rawModule->GetName();
        std::string modVer = rawModule->GetVersion();

        logger.Info("ModuleManager: Initializing dynamic module '" + modName + "' v" + modVer);

        if (!rawModule->Initialize()) {
            logger.Error("ModuleManager: Failed to initialize module '" + modName + "'");
            destroyFn(rawModule);
            return false;
        }

        ModuleDeleter deleter{ destroyFn };
        ManagedModulePtr managedPtr(rawModule, deleter);

        auto writeTime = std::filesystem::last_write_time(dllPath);

        m_loadedModules.push_back(LoadedModuleEntry{
            .moduleName = modName,
            .libraryPath = dllPath,
            .lastWriteTime = writeTime,
            .libraryHandle = std::move(libHandle),
            .moduleInstance = std::move(managedPtr)
        });

        logger.Info("ModuleManager: Successfully loaded dynamic module '" + modName + "'");
        return true;
    }

    bool ModuleManager::ReloadModule(std::string_view moduleName) {
        auto& logger = Logging::LoggerCore::Instance();
        auto it = std::find_if(m_loadedModules.begin(), m_loadedModules.end(),
            [moduleName](const LoadedModuleEntry& entry) {
                return entry.moduleName == moduleName;
            });

        if (it == m_loadedModules.end()) {
            logger.Warning("ModuleManager: Cannot reload non-loaded module '" + std::string(moduleName) + "'");
            return false;
        }

        std::filesystem::path path = it->libraryPath;
        logger.Info("ModuleManager: Hot-Reloading module '" + std::string(moduleName) + "' from " + path.string());

        UnloadModule(moduleName);
        return LoadModuleFromDll(path);
    }

    size_t ModuleManager::CheckAndHotReloadModules() {
        auto& logger = Logging::LoggerCore::Instance();
        size_t reloadedCount = 0;

        for (size_t i = 0; i < m_loadedModules.size(); ++i) {
            const auto& entry = m_loadedModules[i];
            if (std::filesystem::exists(entry.libraryPath)) {
                auto currentWriteTime = std::filesystem::last_write_time(entry.libraryPath);
                if (currentWriteTime > entry.lastWriteTime) {
                    logger.Info("ModuleManager: Detected binary update for module '" + entry.moduleName + "'");
                    std::string modName = entry.moduleName;
                    if (ReloadModule(modName)) {
                        reloadedCount++;
                    }
                }
            }
        }

        return reloadedCount;
    }

    void ModuleManager::UnloadModule(std::string_view moduleName) {
        auto& logger = Logging::LoggerCore::Instance();
        auto it = std::find_if(m_loadedModules.begin(), m_loadedModules.end(),
            [moduleName](const LoadedModuleEntry& entry) {
                return entry.moduleName == moduleName;
            });

        if (it != m_loadedModules.end()) {
            logger.Info("ModuleManager: Unloading module '" + it->moduleName + "'");
            it->moduleInstance.reset(); // Invokes ModuleDeleter (Shutdown + DestroyModule)
            it->libraryHandle.Free();  // Unloads DLL
            m_loadedModules.erase(it);
        } else {
            logger.Warning("ModuleManager: Attempted to unload non-existent module '" + std::string(moduleName) + "'");
        }
    }

    void ModuleManager::UnloadAllModules() {
        auto& logger = Logging::LoggerCore::Instance();
        if (!m_loadedModules.empty()) {
            logger.Info("ModuleManager: Unloading all (" + std::to_string(m_loadedModules.size()) + ") dynamic modules");

            // Unload in reverse order of loading
            for (auto it = m_loadedModules.rbegin(); it != m_loadedModules.rend(); ++it) {
                it->moduleInstance.reset();
                it->libraryHandle.Free();
            }
            m_loadedModules.clear();
        }
    }

    bool ModuleManager::ExecuteAllModules() {
        auto& logger = Logging::LoggerCore::Instance();
        bool allSuccess = true;

        for (auto& entry : m_loadedModules) {
            if (entry.moduleInstance) {
                logger.Info("ModuleManager: Executing module '" + entry.moduleName + "'");
                if (!entry.moduleInstance->Execute()) {
                    logger.Error("ModuleManager: Module '" + entry.moduleName + "' returned failure during execution.");
                    allSuccess = false;
                }
            }
        }

        return allSuccess;
    }

} // namespace SysCore::Modules
