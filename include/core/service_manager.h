#ifndef SYSCORE_SERVICE_MANAGER_H
#define SYSCORE_SERVICE_MANAGER_H

#include "handle.h"
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>

namespace SysCore::Services {

    struct ServiceStatusInfo {
        std::string serviceName;
        std::string displayName;
        DWORD currentState{0};
        DWORD processId{0};
        bool isRunning{false};
    };

    class ServiceManager {
    public:
        [[nodiscard]] static std::vector<ServiceStatusInfo> EnumActiveServices() noexcept;
        [[nodiscard]] static bool IsServiceRunning(const std::string& serviceName) noexcept;
    };

} // namespace SysCore::Services

#endif // SYSCORE_SERVICE_MANAGER_H
