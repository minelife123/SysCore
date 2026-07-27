#ifndef SYSCORE_PROCESS_TREE_H
#define SYSCORE_PROCESS_TREE_H

#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace SysCore::Diagnostics {

    struct ProcessNodeInfo {
        DWORD processId{0};
        DWORD parentProcessId{0};
        std::string processName;
        DWORD threadCount{0};
        size_t workingSetSizeBytes{0};
        bool isElevated{false};
        std::vector<DWORD> threadIds;
        std::vector<std::shared_ptr<ProcessNodeInfo>> children;
    };

    class ProcessTreeManager {
    public:
        [[nodiscard]] static std::vector<std::shared_ptr<ProcessNodeInfo>> BuildProcessTree() noexcept;
        [[nodiscard]] static std::vector<ProcessNodeInfo> GetFlatProcessList() noexcept;
    };

} // namespace SysCore::Diagnostics

#endif // SYSCORE_PROCESS_TREE_H
