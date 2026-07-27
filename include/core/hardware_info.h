#ifndef SYSCORE_HARDWARE_INFO_H
#define SYSCORE_HARDWARE_INFO_H

#include <windows.h>
#include <string>
#include <cstdint>

namespace SysCore::Hardware {

    struct CpuTopologyInfo {
        uint32_t logicalCoreCount{0};
        uint32_t physicalCoreCount{0};
        uint32_t numaNodeCount{0};
        uint64_t l3CacheSizeBytes{0};
        std::string cpuBrandName;
    };

    struct OsVersionInfo {
        DWORD majorVersion{0};
        DWORD minorVersion{0};
        DWORD buildNumber{0};
        std::string osName;
    };

    class HardwareInfo {
    public:
        [[nodiscard]] static CpuTopologyInfo GetCpuTopology() noexcept;
        [[nodiscard]] static OsVersionInfo GetOsVersion() noexcept;
    };

} // namespace SysCore::Hardware

#endif // SYSCORE_HARDWARE_INFO_H
