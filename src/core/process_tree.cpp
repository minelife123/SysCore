#include "core/process_tree.h"
#include <psapi.h>
#include <unordered_map>
#include <algorithm>

namespace SysCore::Diagnostics {

    static size_t GetWorkingSetBytes(DWORD pid)
    {
        HANDLE hProc = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!hProc) return 0;

        PROCESS_MEMORY_COUNTERS pmc{};
        pmc.cb = sizeof(pmc);
        if (::GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc)))
        {
            ::CloseHandle(hProc);
            return pmc.WorkingSetSize;
        }
        ::CloseHandle(hProc);
        return 0;
    }

    std::vector<ProcessNodeInfo> ProcessTreeManager::GetFlatProcessList() noexcept
    {
        std::vector<ProcessNodeInfo> list;
        HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return list;

        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);

        if (::Process32FirstW(hSnapshot, &pe))
        {
            do
            {
                ProcessNodeInfo node{};
                node.processId = pe.th32ProcessID;
                node.parentProcessId = pe.th32ParentProcessID;

                char exeBuf[MAX_PATH] = { 0 };
                ::WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, exeBuf, MAX_PATH, nullptr, nullptr);
                node.processName = std::string(exeBuf);
                node.threadCount = pe.cntThreads;
                node.workingSetSizeBytes = GetWorkingSetBytes(pe.th32ProcessID);

                list.push_back(node);
            } while (::Process32NextW(hSnapshot, &pe));
        }
        ::CloseHandle(hSnapshot);
        return list;
    }

    std::vector<std::shared_ptr<ProcessNodeInfo>> ProcessTreeManager::BuildProcessTree() noexcept
    {
        auto flatList = GetFlatProcessList();
        std::unordered_map<DWORD, std::shared_ptr<ProcessNodeInfo>> map;
        std::vector<std::shared_ptr<ProcessNodeInfo>> roots;

        for (const auto& item : flatList)
        {
            auto ptr = std::make_shared<ProcessNodeInfo>(item);
            map[item.processId] = ptr;
        }

        for (const auto& pair : map)
        {
            auto node = pair.second;
            auto parentIt = map.find(node->parentProcessId);
            if (parentIt != map.end() && parentIt->second->processId != node->processId)
            {
                parentIt->second->children.push_back(node);
            }
            else
            {
                roots.push_back(node);
            }
        }

        return roots;
    }

} // namespace SysCore::Diagnostics
