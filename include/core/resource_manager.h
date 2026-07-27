#ifndef SYSCORE_RESOURCE_MANAGER_H
#define SYSCORE_RESOURCE_MANAGER_H

#include "handle.h"
#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <utility>
#include <memory>

namespace SysCore::Resources {

    // RAII wrapper for WinAPI VirtualAlloc / VirtualFree allocations
    class ScopedVirtualMem {
    private:
        void* m_ptr{nullptr};
        size_t m_size{0};
        HANDLE m_hProcess{::GetCurrentProcess()};

    public:
        ScopedVirtualMem() noexcept = default;
        ScopedVirtualMem(size_t size, DWORD allocationType = MEM_COMMIT | MEM_RESERVE, DWORD protect = PAGE_READWRITE, HANDLE hProcess = ::GetCurrentProcess()) noexcept;
        ~ScopedVirtualMem() noexcept;

        // Non-copyable
        ScopedVirtualMem(const ScopedVirtualMem&) = delete;
        ScopedVirtualMem& operator=(const ScopedVirtualMem&) = delete;

        // Move constructor & assignment
        ScopedVirtualMem(ScopedVirtualMem&& other) noexcept;
        ScopedVirtualMem& operator=(ScopedVirtualMem&& other) noexcept;

        void Free() noexcept;

        [[nodiscard]] void* Get() const noexcept { return m_ptr; }
        [[nodiscard]] size_t GetSize() const noexcept { return m_size; }
        [[nodiscard]] bool IsValid() const noexcept { return m_ptr != nullptr; }

        template <typename T>
        [[nodiscard]] T* As() const noexcept {
            return static_cast<T*>(m_ptr);
        }

        explicit operator bool() const noexcept { return IsValid(); }
    };

    // RAII wrapper for WinAPI HeapAlloc / HeapFree allocations
    class ScopedHeapMem {
    private:
        HANDLE m_hHeap{nullptr};
        void* m_ptr{nullptr};

    public:
        ScopedHeapMem() noexcept = default;
        explicit ScopedHeapMem(size_t size, DWORD flags = 0, HANDLE hHeap = ::GetProcessHeap()) noexcept;
        ~ScopedHeapMem() noexcept;

        ScopedHeapMem(const ScopedHeapMem&) = delete;
        ScopedHeapMem& operator=(const ScopedHeapMem&) = delete;

        ScopedHeapMem(ScopedHeapMem&& other) noexcept;
        ScopedHeapMem& operator=(ScopedHeapMem&& other) noexcept;

        void Free() noexcept;

        [[nodiscard]] void* Get() const noexcept { return m_ptr; }
        [[nodiscard]] bool IsValid() const noexcept { return m_ptr != nullptr; }

        template <typename T>
        [[nodiscard]] T* As() const noexcept {
            return static_cast<T*>(m_ptr);
        }
    };

    // High-level Process Resource Manager using safe RAII handles
    class ProcessManager {
    public:
        // Open process by ID returning RAII ProcessHandle wrapper
        [[nodiscard]] static Core::ProcessHandle OpenProcess(DWORD processId, DWORD desiredAccess = PROCESS_ALL_ACCESS) noexcept;

        // Read process memory safely into buffer
        static bool ReadMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead = nullptr) noexcept;

        // Write process memory safely from buffer
        static bool WriteMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten = nullptr) noexcept;

        // Get current process architecture (x64 verification)
        [[nodiscard]] static bool IsCurrentProcess64Bit() noexcept;
    };

} // namespace SysCore::Resources

#endif // SYSCORE_RESOURCE_MANAGER_H
