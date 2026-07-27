#include "core/token_manager.h"
#include "core/logger.h"

namespace SysCore::Security {

    bool TokenManager::IsCurrentProcessElevated() noexcept {
        HANDLE hToken = nullptr;
        if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            return false;
        }

        Core::ProcessHandle tokenHandle(hToken);
        TOKEN_ELEVATION elevation{};
        DWORD size = sizeof(TOKEN_ELEVATION);

        if (::GetTokenInformation(tokenHandle.Get(), TokenElevation, &elevation, sizeof(elevation), &size)) {
            return elevation.TokenIsElevated != 0;
        }

        return false;
    }

    ProcessSecurityContext TokenManager::GetCurrentProcessSecurityContext() noexcept {
        ProcessSecurityContext ctx{};
        ctx.isElevated = IsCurrentProcessElevated();

        HANDLE hToken = nullptr;
        if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            return ctx;
        }

        Core::ProcessHandle tokenHandle(hToken);

        // Audit Integrity Level
        DWORD bytesNeeded = 0;
        ::GetTokenInformation(tokenHandle.Get(), TokenIntegrityLevel, nullptr, 0, &bytesNeeded);
        if (bytesNeeded > 0) {
            std::vector<BYTE> buffer(bytesNeeded);
            if (::GetTokenInformation(tokenHandle.Get(), TokenIntegrityLevel, buffer.data(), bytesNeeded, &bytesNeeded)) {
                auto pLabel = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(buffer.data());
                DWORD subAuth = *::GetSidSubAuthority(pLabel->Label.Sid, (DWORD)(UCHAR)(*::GetSidSubAuthorityCount(pLabel->Label.Sid) - 1));

                ctx.integrityLevel = subAuth;
                if (subAuth >= SECURITY_MANDATORY_HIGH_RID) {
                    ctx.integrityLevelName = "High (Administrator)";
                } else if (subAuth >= SECURITY_MANDATORY_MEDIUM_RID) {
                    ctx.integrityLevelName = "Medium (Standard User)";
                } else if (subAuth >= SECURITY_MANDATORY_LOW_RID) {
                    ctx.integrityLevelName = "Low (Restricted Sandbox)";
                } else {
                    ctx.integrityLevelName = "Untrusted";
                }
            }
        }

        // Audit Privileges
        bytesNeeded = 0;
        ::GetTokenInformation(tokenHandle.Get(), TokenPrivileges, nullptr, 0, &bytesNeeded);
        if (bytesNeeded > 0) {
            std::vector<BYTE> buffer(bytesNeeded);
            if (::GetTokenInformation(tokenHandle.Get(), TokenPrivileges, buffer.data(), bytesNeeded, &bytesNeeded)) {
                auto pPrivs = reinterpret_cast<TOKEN_PRIVILEGES*>(buffer.data());
                ctx.privileges.reserve(pPrivs->PrivilegeCount);

                for (DWORD i = 0; i < pPrivs->PrivilegeCount; ++i) {
                    char nameBuf[256]{0};
                    DWORD nameLen = sizeof(nameBuf);
                    if (::LookupPrivilegeNameA(nullptr, &pPrivs->Privileges[i].Luid, nameBuf, &nameLen)) {
                        TokenPrivilegeInfo info{};
                        info.privilegeName = nameBuf;
                        info.isEnabled = (pPrivs->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) != 0;
                        ctx.privileges.push_back(info);
                    }
                }
            }
        }

        return ctx;
    }

} // namespace SysCore::Security
