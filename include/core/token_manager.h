#ifndef SYSCORE_TOKEN_MANAGER_H
#define SYSCORE_TOKEN_MANAGER_H

#include "handle.h"
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>

namespace SysCore::Security {

    struct TokenPrivilegeInfo {
        std::string privilegeName;
        bool isEnabled{false};
    };

    struct ProcessSecurityContext {
        bool isElevated{false};
        DWORD integrityLevel{0};
        std::string integrityLevelName;
        std::vector<TokenPrivilegeInfo> privileges;
    };

    class TokenManager {
    public:
        [[nodiscard]] static ProcessSecurityContext GetCurrentProcessSecurityContext() noexcept;
        [[nodiscard]] static bool IsCurrentProcessElevated() noexcept;
    };

} // namespace SysCore::Security

#endif // SYSCORE_TOKEN_MANAGER_H
