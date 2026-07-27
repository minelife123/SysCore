#ifndef PROCESS_WATCHER_MODULE_H
#define PROCESS_WATCHER_MODULE_H

#include "core/imodule.h"
#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>

struct ProcessSnapshotMetrics {
    size_t activeProcessCount{0};
    uint32_t currentPid{0};
};

class ProcessWatcherModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};

public:
    ProcessWatcherModule() = default;
    ~ProcessWatcherModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "ProcessWatcherModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "4.0.0"; }

    [[nodiscard]] ProcessSnapshotMetrics CountActiveProcesses() const noexcept;
};

#endif // PROCESS_WATCHER_MODULE_H
