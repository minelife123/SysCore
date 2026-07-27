#ifndef SYSCORE_HANDLE_H
#define SYSCORE_HANDLE_H

#include "concepts.h"
#include <windows.h>
#include <utility>

namespace SysCore::Core {

    // Standard traits for Win32 kernel objects closed via CloseHandle
    struct StandardHandleTraits {
        static HANDLE InvalidValue() noexcept { return NULL; }
        static void Close(HANDLE h) noexcept {
            if (h != NULL && h != INVALID_HANDLE_VALUE) {
                ::CloseHandle(h);
            }
        }
    };

    // Standard traits for Win32 file/snapshot objects closed via CloseHandle with INVALID_HANDLE_VALUE
    struct InvalidHandleValueTraits {
        static HANDLE InvalidValue() noexcept { return INVALID_HANDLE_VALUE; }
        static void Close(HANDLE h) noexcept {
            if (h != NULL && h != INVALID_HANDLE_VALUE) {
                ::CloseHandle(h);
            }
        }
    };

    // Generic RAII Handle wrapper using C++20 concepts
    template <Concepts::HandleTraitsConcept Traits = StandardHandleTraits>
    class UniqueHandle {
    private:
        HANDLE m_handle;

        void Cleanup() noexcept {
            if (IsValid()) {
                Traits::Close(m_handle);
                m_handle = Traits::InvalidValue();
            }
        }

    public:
        constexpr UniqueHandle() noexcept : m_handle(Traits::InvalidValue()) {}
        explicit UniqueHandle(HANDLE handle) noexcept : m_handle(handle) {}

        ~UniqueHandle() noexcept {
            Cleanup();
        }

        // Non-copyable (prevent duplicate closing of raw handles)
        UniqueHandle(const UniqueHandle&) = delete;
        UniqueHandle& operator=(const UniqueHandle&) = delete;

        // Move constructor
        UniqueHandle(UniqueHandle&& other) noexcept : m_handle(other.m_handle) {
            other.m_handle = Traits::InvalidValue();
        }

        // Move assignment
        UniqueHandle& operator=(UniqueHandle&& other) noexcept {
            if (this != &other) {
                Cleanup();
                m_handle = other.m_handle;
                other.m_handle = Traits::InvalidValue();
            }
            return *this;
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return m_handle != Traits::InvalidValue() && m_handle != NULL;
        }

        [[nodiscard]] HANDLE Get() const noexcept {
            return m_handle;
        }

        [[nodiscard]] HANDLE* GetAddressOf() noexcept {
            Cleanup();
            return &m_handle;
        }

        HANDLE Release() noexcept {
            HANDLE temp = m_handle;
            m_handle = Traits::InvalidValue();
            return temp;
        }

        void Reset(HANDLE newHandle = Traits::InvalidValue()) noexcept {
            if (m_handle != newHandle) {
                Cleanup();
                m_handle = newHandle;
            }
        }

        explicit operator bool() const noexcept {
            return IsValid();
        }
    };

    using ProcessHandle = UniqueHandle<StandardHandleTraits>;
    using ThreadHandle = UniqueHandle<StandardHandleTraits>;
    using FileHandle = UniqueHandle<InvalidHandleValueTraits>;

} // namespace SysCore::Core

#endif // SYSCORE_HANDLE_H
