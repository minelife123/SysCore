#include "core/service_manager.h"
#include "core/logger.h"

namespace SysCore::Services {

    std::vector<ServiceStatusInfo> ServiceManager::EnumActiveServices() noexcept {
        std::vector<ServiceStatusInfo> list;

        SC_HANDLE hScm = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_CONNECT);
        if (!hScm) return list;

        DWORD bytesNeeded = 0, serviceCount = 0, resumeHandle = 0;
        ::EnumServicesStatusExW(
            hScm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
            nullptr, 0, &bytesNeeded, &serviceCount, &resumeHandle, nullptr);

        if (bytesNeeded > 0) {
            std::vector<BYTE> buffer(bytesNeeded);
            if (::EnumServicesStatusExW(
                hScm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
                buffer.data(), bytesNeeded, &bytesNeeded, &serviceCount, &resumeHandle, nullptr)) {

                auto services = reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSW*>(buffer.data());
                list.reserve(serviceCount);

                for (DWORD i = 0; i < serviceCount; ++i) {
                    ServiceStatusInfo info{};
                    char nameBuf[256]{0};
                    char displayBuf[256]{0};

                    ::WideCharToMultiByte(CP_UTF8, 0, services[i].lpServiceName, -1, nameBuf, sizeof(nameBuf), nullptr, nullptr);
                    ::WideCharToMultiByte(CP_UTF8, 0, services[i].lpDisplayName, -1, displayBuf, sizeof(displayBuf), nullptr, nullptr);

                    info.serviceName = nameBuf;
                    info.displayName = displayBuf;
                    info.currentState = services[i].ServiceStatusProcess.dwCurrentState;
                    info.processId = services[i].ServiceStatusProcess.dwProcessId;
                    info.isRunning = (info.currentState == SERVICE_RUNNING);

                    list.push_back(info);
                }
            }
        }

        ::CloseServiceHandle(hScm);
        return list;
    }

    bool ServiceManager::IsServiceRunning(const std::string& serviceName) noexcept {
        auto services = EnumActiveServices();
        for (const auto& svc : services) {
            if (svc.serviceName == serviceName && svc.isRunning) {
                return true;
            }
        }
        return false;
    }

} // namespace SysCore::Services
