#ifndef SYSCORE_MODULE_MANAGER_H
#define SYSCORE_MODULE_MANAGER_H

#include "imodule.h"
#include <windows.h>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <filesystem>

namespace SysCore::Modules {

    // RAII wrapper for dynamic libraries (HMODULE)
    class LibraryHandle {
    private:
        HMODULE m_hModule{nullptr};

    public:
        LibraryHandle() noexcept = default;
        explicit LibraryHandle(HMODULE hModule) noexcept : m_hModule(hModule) {}
        ~LibraryHandle() noexcept {
            Free();
        }

        LibraryHandle(const LibraryHandle&) = delete;
        LibraryHandle& operator=(const LibraryHandle&) = delete;

        LibraryHandle(LibraryHandle&& other) noexcept : m_hModule(other.m_hModule) {
            other.m_hModule = nullptr;
        }

        LibraryHandle& operator=(LibraryHandle&& other) noexcept {
            if (this != &other) {
                Free();
                m_hModule = other.m_hModule;
                other.m_hModule = nullptr;
            }
            return *this;
        }

        void Free() noexcept {
            if (m_hModule != nullptr) {
                ::FreeLibrary(m_hModule);
                m_hModule = nullptr;
            }
        }

        [[nodiscard]] bool IsValid() const noexcept { return m_hModule != nullptr; }
        [[nodiscard]] HMODULE Get() const noexcept { return m_hModule; }

        template <typename T>
        [[nodiscard]] T GetFunction(const char* funcName) const noexcept {
            if (!m_hModule) return nullptr;
            return reinterpret_cast<T>(::GetProcAddress(m_hModule, funcName));
        }

        explicit operator bool() const noexcept { return IsValid(); }
    };

    // Custom deleter for dynamic IModule instances ensuring proper DLL destructor execution
    struct ModuleDeleter {
        Interfaces::DestroyModuleFn destroyFunc{nullptr};

        void operator()(Interfaces::IModule* modulePtr) const noexcept {
            if (modulePtr) {
                modulePtr->Shutdown();
                if (destroyFunc) {
                    destroyFunc(modulePtr);
                } else {
                    delete modulePtr;
                }
            }
        }
    };

    using ManagedModulePtr = std::unique_ptr<Interfaces::IModule, ModuleDeleter>;

    struct LoadedModuleEntry {
        std::string moduleName;
        std::filesystem::path libraryPath;
        std::filesystem::file_time_type lastWriteTime;
        LibraryHandle libraryHandle;
        ManagedModulePtr moduleInstance;
    };

    // Class for dynamic component registration and lifecycle execution
    class ModuleManager {
    private:
        std::vector<LoadedModuleEntry> m_loadedModules;

    public:
        ModuleManager() = default;
        ~ModuleManager();

        ModuleManager(const ModuleManager&) = delete;
        ModuleManager& operator=(const ModuleManager&) = delete;

        // Dynamic Loading & Hot-Reload API
        [[nodiscard]] bool LoadModuleFromDll(const std::filesystem::path& dllPath);
        bool ReloadModule(std::string_view moduleName);
        size_t CheckAndHotReloadModules();
        void UnloadModule(std::string_view moduleName);
        void UnloadAllModules();

        // Execution & Iteration API
        bool ExecuteAllModules();
        [[nodiscard]] size_t GetLoadedModuleCount() const noexcept { return m_loadedModules.size(); }
        [[nodiscard]] const std::vector<LoadedModuleEntry>& GetLoadedModules() const noexcept { return m_loadedModules; }
    };

} // namespace SysCore::Modules

#endif // SYSCORE_MODULE_MANAGER_H
