#include "core/resource_manager.h"
#include "core/logger.h"

namespace SysCore::Resources {

    // --- ScopedVirtualMem Implementation ---

    ScopedVirtualMem::ScopedVirtualMem(size_t size, DWORD allocationType, DWORD protect, HANDLE hProcess) noexcept
        : m_size(size), m_hProcess(hProcess) {
        if (size > 0) {
            if (hProcess == ::GetCurrentProcess()) {
                m_ptr = ::VirtualAlloc(nullptr, size, allocationType, protect);
            } else {
                m_ptr = ::VirtualAllocEx(hProcess, nullptr, size, allocationType, protect);
            }

            if (!m_ptr) {
                Logging::LoggerCore::Instance().Error(
                    "ScopedVirtualMem: VirtualAlloc failed with error code " + std::to_string(::GetLastError()));
            }
        }
    }

    ScopedVirtualMem::~ScopedVirtualMem() noexcept {
        Free();
    }

    ScopedVirtualMem::ScopedVirtualMem(ScopedVirtualMem&& other) noexcept
        : m_ptr(other.m_ptr), m_size(other.m_size), m_hProcess(other.m_hProcess) {
        other.m_ptr = nullptr;
        other.m_size = 0;
        other.m_hProcess = nullptr;
    }

    ScopedVirtualMem& ScopedVirtualMem::operator=(ScopedVirtualMem&& other) noexcept {
        if (this != &other) {
            Free();
            m_ptr = other.m_ptr;
            m_size = other.m_size;
            m_hProcess = other.m_hProcess;

            other.m_ptr = nullptr;
            other.m_size = 0;
            other.m_hProcess = nullptr;
        }
        return *this;
    }

    void ScopedVirtualMem::Free() noexcept {
        if (m_ptr != nullptr) {
            if (m_hProcess == ::GetCurrentProcess()) {
                ::VirtualFree(m_ptr, 0, MEM_RELEASE);
            } else if (m_hProcess != nullptr) {
                ::VirtualFreeEx(m_hProcess, m_ptr, 0, MEM_RELEASE);
            }
            m_ptr = nullptr;
            m_size = 0;
        }
    }

    // --- ScopedHeapMem Implementation ---

    ScopedHeapMem::ScopedHeapMem(size_t size, DWORD flags, HANDLE hHeap) noexcept
        : m_hHeap(hHeap) {
        if (size > 0 && hHeap != nullptr) {
            m_ptr = ::HeapAlloc(hHeap, flags, size);
            if (!m_ptr) {
                Logging::LoggerCore::Instance().Error(
                    "ScopedHeapMem: HeapAlloc failed with error code " + std::to_string(::GetLastError()));
            }
        }
    }

    ScopedHeapMem::~ScopedHeapMem() noexcept {
        Free();
    }

    ScopedHeapMem::ScopedHeapMem(ScopedHeapMem&& other) noexcept
        : m_hHeap(other.m_hHeap), m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
        other.m_hHeap = nullptr;
    }

    ScopedHeapMem& ScopedHeapMem::operator=(ScopedHeapMem&& other) noexcept {
        if (this != &other) {
            Free();
            m_hHeap = other.m_hHeap;
            m_ptr = other.m_ptr;

            other.m_ptr = nullptr;
            other.m_hHeap = nullptr;
        }
        return *this;
    }

    void ScopedHeapMem::Free() noexcept {
        if (m_ptr != nullptr && m_hHeap != nullptr) {
            ::HeapFree(m_hHeap, 0, m_ptr);
            m_ptr = nullptr;
        }
    }

    // --- ProcessManager Implementation ---

    Core::ProcessHandle ProcessManager::OpenProcess(DWORD processId, DWORD desiredAccess) noexcept {
        HANDLE hProc = ::OpenProcess(desiredAccess, FALSE, processId);
        if (!hProc) {
            Logging::LoggerCore::Instance().Warning(
                "ProcessManager::OpenProcess failed for PID " + std::to_string(processId) +
                " with error " + std::to_string(::GetLastError()));
        }
        return Core::ProcessHandle(hProc);
    }

    bool ProcessManager::ReadMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead) noexcept {
        if (!hProcess || !lpBaseAddress || !lpBuffer || nSize == 0) return false;
        BOOL result = ::ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
        return result != FALSE;
    }

    bool ProcessManager::WriteMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten) noexcept {
        if (!hProcess || !lpBaseAddress || !lpBuffer || nSize == 0) return false;
        BOOL result = ::WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
        return result != FALSE;
    }

    bool ProcessManager::IsCurrentProcess64Bit() noexcept {
#if defined(_WIN64)
        return true;
#else
        BOOL isWow64 = FALSE;
        typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
        LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)::GetProcAddress(
            ::GetModuleHandleW(L"kernel32"), "IsWow64Process");
        if (fnIsWow64Process) {
            fnIsWow64Process(::GetCurrentProcess(), &isWow64);
        }
        return isWow64 != FALSE;
#endif
    }

} // namespace SysCore::Resources
