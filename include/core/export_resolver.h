#ifndef SYSCORE_EXPORT_RESOLVER_H
#define SYSCORE_EXPORT_RESOLVER_H

#include <windows.h>
#include <string_view>
#include <string>
#include <stdexcept>
#include <utility>

namespace SysCore::Utils {

    class ExportResolver {
    public:
        // Resolve function export address by name with forwarded exports and ordinal support
        static uintptr_t ResolveExportAddress(HMODULE hModule, std::string_view functionName) noexcept {
            if (!hModule || functionName.empty()) return 0;
            auto baseAddress = reinterpret_cast<const unsigned char*>(hModule);
            
            auto dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(baseAddress);
            if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return 0;
            
            auto ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS64*>(baseAddress + dosHeader->e_lfanew);
            if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return 0;
            
            const auto& exportDataDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
            DWORD exportDirRVA = exportDataDir.VirtualAddress;
            DWORD exportDirSize = exportDataDir.Size;
            if (!exportDirRVA || !exportDirSize) return 0;
            
            auto exportDirectory = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(baseAddress + exportDirRVA);
            auto functions = reinterpret_cast<const DWORD*>(baseAddress + exportDirectory->AddressOfFunctions);
            auto names = reinterpret_cast<const DWORD*>(baseAddress + exportDirectory->AddressOfNames);
            auto ordinals = reinterpret_cast<const WORD*>(baseAddress + exportDirectory->AddressOfNameOrdinals);
            
            for (DWORD i = 0; i < exportDirectory->NumberOfNames; ++i) {
                const char* name = reinterpret_cast<const char*>(baseAddress + names[i]);
                if (functionName == name) {
                    WORD ordinal = ordinals[i];
                    DWORD functionRVA = functions[ordinal];
                    
                    // Check for Forwarded Export (RVA inside Export Directory range)
                    if (functionRVA >= exportDirRVA && functionRVA < (exportDirRVA + exportDirSize)) {
                        const char* forwardString = reinterpret_cast<const char*>(baseAddress + functionRVA);
                        std::string_view forward(forwardString);
                        
                        auto dotPos = forward.find('.');
                        if (dotPos == std::string_view::npos) return 0;
                        
                        std::string targetDll(forward.substr(0, dotPos));
                        targetDll += ".dll";
                        std::string targetFunc(forward.substr(dotPos + 1));
                        
                        // Check if module is already loaded first
                        HMODULE hTargetModule = ::GetModuleHandleA(targetDll.c_str());
                        if (!hTargetModule) {
                            hTargetModule = ::LoadLibraryA(targetDll.c_str());
                        }
                        if (!hTargetModule) return 0;
                        
                        // Ordinal forwarded export support (e.g. "NTDLL.#12")
                        if (!targetFunc.empty() && targetFunc[0] == '#') {
                            WORD targetOrdinal = static_cast<WORD>(std::stoul(targetFunc.substr(1)));
                            return ResolveExportByOrdinal(hTargetModule, targetOrdinal);
                        }
                        
                        // Recursive resolution
                        return ResolveExportAddress(hTargetModule, targetFunc);
                    }
                    
                    return reinterpret_cast<uintptr_t>(baseAddress + functionRVA);
                }
            }
            return 0;
        }

        // Resolve function export by ordinal index
        static uintptr_t ResolveExportByOrdinal(HMODULE hModule, WORD ordinal) noexcept {
            if (!hModule) return 0;
            auto baseAddress = reinterpret_cast<const unsigned char*>(hModule);
            
            auto dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(baseAddress);
            if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return 0;
            
            auto ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS64*>(baseAddress + dosHeader->e_lfanew);
            const auto& exportDataDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
            if (!exportDataDir.VirtualAddress) return 0;

            auto exportDirectory = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(baseAddress + exportDataDir.VirtualAddress);
            auto functions = reinterpret_cast<const DWORD*>(baseAddress + exportDirectory->AddressOfFunctions);

            DWORD functionIndex = ordinal - exportDirectory->Base;
            if (functionIndex >= exportDirectory->NumberOfFunctions) return 0;

            DWORD functionRVA = functions[functionIndex];
            return reinterpret_cast<uintptr_t>(baseAddress + functionRVA);
        }

        // Invoke function dynamically after resolving its export address
        template <typename ReturnType, typename... Args>
        static ReturnType InvokeDynamicCall(HMODULE hModule, std::string_view functionName, Args&&... args) {
            uintptr_t address = ResolveExportAddress(hModule, functionName);
            if (!address) {
                throw std::runtime_error("ExportResolver: Failed to resolve symbol address: " + std::string(functionName));
            }
            
            using FunctionFn = ReturnType(*)(Args...);
            auto targetFunction = reinterpret_cast<FunctionFn>(address);
            return targetFunction(std::forward<Args>(args)...);
        }
    };

} // namespace SysCore::Utils

#endif // SYSCORE_EXPORT_RESOLVER_H
