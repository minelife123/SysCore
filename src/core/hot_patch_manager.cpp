#include "core/hot_patch_manager.h"
#include "core/logger.h"
#include <cstring>

namespace SysCore::Instrumentation {

    bool HotPatchManager::InstallHook(void* targetFunc, void* detourFunc) {
        if (!targetFunc || !detourFunc) return false;

        std::unique_lock lock(m_mutex);
        if (m_hooks.contains(targetFunc) && m_hooks[targetFunc].isInstalled) {
            return false; // Already hooked
        }

        HookEntry entry{};
        entry.targetFunc = targetFunc;
        entry.detourFunc = detourFunc;

        // 1. Change memory protection to PAGE_EXECUTE_READWRITE for 5 bytes
        ScopedMemProtect memProtect(targetFunc, 5, PAGE_EXECUTE_READWRITE);
        if (!memProtect.IsSuccess()) {
            Logging::LoggerCore::Instance().Error("HotPatchManager: Failed to set PAGE_EXECUTE_READWRITE protection");
            return false;
        }

        // 2. Backup original 5 bytes
        std::memcpy(entry.originalBytes, targetFunc, 5);

        // 3. Compute 32-bit relative jump offset: Offset = Detour - Target - 5
        intptr_t relOffset = reinterpret_cast<intptr_t>(detourFunc) - reinterpret_cast<intptr_t>(targetFunc) - 5;

        // Ensure 32-bit relative jump is within range
        if (relOffset < -2147483647LL || relOffset > 2147483647LL) {
            Logging::LoggerCore::Instance().Error("HotPatchManager: Relative jump offset out of 32-bit range");
            return false;
        }

        uint8_t patch[5];
        patch[0] = 0xE9; // JMP opcode
        *reinterpret_cast<int32_t*>(patch + 1) = static_cast<int32_t>(relOffset);

        // 4. Overwrite target function head with 5-byte JMP patch
        std::memcpy(targetFunc, patch, 5);
        ::FlushInstructionCache(::GetCurrentProcess(), targetFunc, 5);
        entry.isInstalled = true;

        m_hooks[targetFunc] = entry;
        Logging::LoggerCore::Instance().Info("HotPatchManager: Successfully installed function detour hook.");
        return true;
    }

    bool HotPatchManager::RemoveHook(void* targetFunc) {
        if (!targetFunc) return false;

        std::unique_lock lock(m_mutex);
        auto it = m_hooks.find(targetFunc);
        if (it == m_hooks.end() || !it->second.isInstalled) {
            return false;
        }

        auto& entry = it->second;

        // Restore original 5 bytes
        ScopedMemProtect memProtect(targetFunc, 5, PAGE_EXECUTE_READWRITE);
        if (!memProtect.IsSuccess()) return false;

        std::memcpy(targetFunc, entry.originalBytes, 5);
        ::FlushInstructionCache(::GetCurrentProcess(), targetFunc, 5);
        entry.isInstalled = false;
        m_hooks.erase(it);

        Logging::LoggerCore::Instance().Info("HotPatchManager: Successfully removed function detour hook.");
        return true;
    }

    void HotPatchManager::RemoveAllHooks() {
        std::unique_lock lock(m_mutex);
        for (auto& [targetFunc, entry] : m_hooks) {
            if (entry.isInstalled && targetFunc) {
                ScopedMemProtect memProtect(targetFunc, 5, PAGE_EXECUTE_READWRITE);
                if (memProtect.IsSuccess()) {
                    std::memcpy(targetFunc, entry.originalBytes, 5);
                }
            }
        }
        m_hooks.clear();
    }

    bool HotPatchManager::IsHooked(void* targetFunc) const {
        std::shared_lock lock(m_mutex);
        auto it = m_hooks.find(targetFunc);
        return (it != m_hooks.end() && it->second.isInstalled);
    }

} // namespace SysCore::Instrumentation
