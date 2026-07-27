#ifndef SYSCORE_IMODULE_H
#define SYSCORE_IMODULE_H

#include <cstdint>

namespace SysCore::Interfaces {

    // Abstract C++20 interface for dynamically loaded system modules
    class IModule {
    public:
        virtual ~IModule() = default;

        // Module lifecycle contracts
        [[nodiscard]] virtual bool Initialize() = 0;
        [[nodiscard]] virtual bool Execute() = 0;
        virtual void Shutdown() = 0;

        // Metadata providers
        [[nodiscard]] virtual const char* GetName() const noexcept = 0;
        [[nodiscard]] virtual const char* GetVersion() const noexcept = 0;
    };

    // Factory signature definitions for DLL exports
    using CreateModuleFn = IModule* (*)();
    using DestroyModuleFn = void (*)(IModule*);

} // namespace SysCore::Interfaces

// Macro helper for dynamic library exports
#ifdef _WIN32
    #define SYSCORE_MODULE_API extern "C" __declspec(dllexport)
#else
    #define SYSCORE_MODULE_API extern "C"
#endif

#endif // SYSCORE_IMODULE_H
