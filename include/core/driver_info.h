#ifndef SYSCORE_DRIVER_INFO_H
#define SYSCORE_DRIVER_INFO_H

#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>

namespace SysCore::Kernel {

    struct DeviceDriverEntry {
        std::string baseName;
        std::string fileName;
        uintptr_t loadAddress{0};
    };

    class DriverInfo {
    public:
        [[nodiscard]] static std::vector<DeviceDriverEntry> EnumLoadedDrivers() noexcept;
    };

} // namespace SysCore::Kernel

#endif // SYSCORE_DRIVER_INFO_H
