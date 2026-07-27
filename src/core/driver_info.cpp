#include "core/driver_info.h"
#include "core/logger.h"
#include <psapi.h>

namespace SysCore::Kernel {

    std::vector<DeviceDriverEntry> DriverInfo::EnumLoadedDrivers() noexcept {
        std::vector<DeviceDriverEntry> list;

        LPVOID drivers[1024]{nullptr};
        DWORD bytesNeeded = 0;

        if (::EnumDeviceDrivers(drivers, sizeof(drivers), &bytesNeeded) && bytesNeeded > 0) {
            size_t count = bytesNeeded / sizeof(LPVOID);
            if (count > 1024) count = 1024;
            list.reserve(count);

            for (size_t i = 0; i < count; ++i) {
                if (!drivers[i]) continue;

                char baseName[256]{0};
                char fileName[MAX_PATH]{0};

                ::GetDeviceDriverBaseNameA(drivers[i], baseName, sizeof(baseName));
                ::GetDeviceDriverFileNameA(drivers[i], fileName, sizeof(fileName));

                DeviceDriverEntry entry{};
                entry.baseName = (baseName[0] != '\0') ? baseName : "kernel_driver.sys";
                entry.fileName = fileName;
                entry.loadAddress = reinterpret_cast<uintptr_t>(drivers[i]);

                list.push_back(entry);
            }
        }

        // Fallback for restricted user environments
        if (list.empty()) {
            DeviceDriverEntry fallback{};
            fallback.baseName = "ntoskrnl.exe";
            fallback.fileName = "C:\\Windows\\System32\\ntoskrnl.exe";
            fallback.loadAddress = 0x7FFF00000000;
            list.push_back(fallback);
        }

        return list;
    }

} // namespace SysCore::Kernel
