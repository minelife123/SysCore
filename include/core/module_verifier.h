#ifndef SYSCORE_MODULE_VERIFIER_H
#define SYSCORE_MODULE_VERIFIER_H

#include "export_resolver.h"
#include <windows.h>
#include <filesystem>
#include <string>

namespace SysCore::Modules {

    struct ModuleVerificationResult {
        bool isValid{false};
        bool hasCreateModule{false};
        bool hasDestroyModule{false};
        std::string errorMessage;
    };

    class ModuleVerifier {
    public:
        static ModuleVerificationResult VerifyModuleDll(const std::filesystem::path& dllPath) noexcept {
            ModuleVerificationResult result{};

            if (!std::filesystem::exists(dllPath)) {
                result.errorMessage = "Module DLL file does not exist.";
                return result;
            }

            // Load module as datafile/dont-resolve to inspect headers safely without executing DllMain
            HMODULE hMod = ::LoadLibraryExW(dllPath.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
            if (!hMod) {
                result.errorMessage = "Failed to load library headers (Error " + std::to_string(::GetLastError()) + ")";
                return result;
            }

            uintptr_t pCreate = Utils::ExportResolver::ResolveExportAddress(hMod, "CreateModule");
            uintptr_t pDestroy = Utils::ExportResolver::ResolveExportAddress(hMod, "DestroyModule");

            ::FreeLibrary(hMod);

            result.hasCreateModule = (pCreate != 0);
            result.hasDestroyModule = (pDestroy != 0);

            if (result.hasCreateModule && result.hasDestroyModule) {
                result.isValid = true;
            } else {
                result.errorMessage = "Module is missing required C-factory exports (CreateModule/DestroyModule).";
            }

            return result;
        }
    };

} // namespace SysCore::Modules

#endif // SYSCORE_MODULE_VERIFIER_H
