#include "core/hardware_info.h"
#include <intrin.h>
#include <vector>
#include <array>

namespace SysCore::Hardware {

    typedef NTSTATUS(WINAPI* pfnRtlGetVersion)(PRTL_OSVERSIONINFOW);

    CpuTopologyInfo HardwareInfo::GetCpuTopology() noexcept {
        CpuTopologyInfo info{};
        SYSTEM_INFO sysInfo{};
        ::GetSystemInfo(&sysInfo);
        info.logicalCoreCount = sysInfo.dwNumberOfProcessors;

        // Query CPU Brand string via __cpuid
        std::array<int, 4> cpuidData{};
        char brand[49]{0};
        __cpuid(cpuidData.data(), 0x80000000);
        unsigned int nExIds = cpuidData[0];

        if (nExIds >= 0x80000004) {
            for (unsigned int i = 0x80000002; i <= 0x80000004; ++i) {
                __cpuid(cpuidData.data(), i);
                std::memcpy(brand + (i - 0x80000002) * 16, cpuidData.data(), 16);
            }
            info.cpuBrandName = brand;
        } else {
            info.cpuBrandName = "x86_64 Compatible Processor";
        }

        // Query Physical cores via GetLogicalProcessorInformationEx
        DWORD bufferSize = 0;
        if (!::GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bufferSize) &&
            ::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            std::vector<BYTE> buffer(bufferSize);
            if (::GetLogicalProcessorInformationEx(RelationProcessorCore, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &bufferSize)) {
                DWORD offset = 0;
                while (offset < bufferSize) {
                    auto infoEx = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data() + offset);
                    if (infoEx->Relationship == RelationProcessorCore) {
                        info.physicalCoreCount++;
                    }
                    offset += infoEx->Size;
                }
            }
        }

        if (info.physicalCoreCount == 0) {
            info.physicalCoreCount = info.logicalCoreCount / 2;
        }

        return info;
    }

    OsVersionInfo HardwareInfo::GetOsVersion() noexcept {
        OsVersionInfo info{};
        HMODULE hNtdll = ::GetModuleHandleW(L"ntdll.dll");
        if (hNtdll) {
            auto rtlGetVersion = reinterpret_cast<pfnRtlGetVersion>(::GetProcAddress(hNtdll, "RtlGetVersion"));
            if (rtlGetVersion) {
                RTL_OSVERSIONINFOW rovi{};
                rovi.dwOSVersionInfoSize = sizeof(rovi);
                if (rtlGetVersion(&rovi) == 0) {
                    info.majorVersion = rovi.dwMajorVersion;
                    info.minorVersion = rovi.dwMinorVersion;
                    info.buildNumber = rovi.dwBuildNumber;
                    info.osName = "Windows " + std::to_string(info.majorVersion) + " (Build " + std::to_string(info.buildNumber) + ")";
                }
            }
        }
        if (info.majorVersion == 0) {
            info.osName = "Windows x64 OS";
        }
        return info;
    }

} // namespace SysCore::Hardware
