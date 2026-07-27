#ifndef SYSCORE_HOT_PATCH_MANAGER_H
#define SYSCORE_HOT_PATCH_MANAGER_H

#include <windows.h>
#include <cstdint>
#include <vector>
#include <shared_mutex>
#include <unordered_map>

namespace SysCore::Instrumentation {

    class ScopedMemProtect {
    private:
        void* m_address{nullptr};
        SIZE_T m_size{0};
        DWORD m_oldProtect{0};
        bool m_success{false};

    public:
        ScopedMemProtect(void* addr, SIZE_T size, DWORD newProtect)
            : m_address(addr), m_size(size) {
            m_success = (::VirtualProtect(addr, size, newProtect, &m_oldProtect) != FALSE);
        }

        ~ScopedMemProtect() {
            if (m_success) {
                DWORD temp;
                ::VirtualProtect(m_address, m_size, m_oldProtect, &temp);
            }
        }

        [[nodiscard]] bool IsSuccess() const noexcept { return m_success; }
    };

    struct HookEntry {
        void* targetFunc{nullptr};
        void* detourFunc{nullptr};
        uint8_t originalBytes[5]{0};
        bool isInstalled{false};
    };

    class HotPatchManager {
    private:
        std::unordered_map<void*, HookEntry> m_hooks;
        mutable std::shared_mutex m_mutex;

        HotPatchManager() = default;

    public:
        static HotPatchManager& Instance() {
            static HotPatchManager instance;
            return instance;
        }

        HotPatchManager(const HotPatchManager&) = delete;
        HotPatchManager& operator=(const HotPatchManager&) = delete;

        bool InstallHook(void* targetFunc, void* detourFunc);
        bool RemoveHook(void* targetFunc);
        void RemoveAllHooks();

        [[nodiscard]] bool IsHooked(void* targetFunc) const;
    };

} // namespace SysCore::Instrumentation

#endif // SYSCORE_HOT_PATCH_MANAGER_H
