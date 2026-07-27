#ifndef FILE_WATCHER_MODULE_H
#define FILE_WATCHER_MODULE_H

#include "core/imodule.h"
#include "core/file_watcher.h"

class FileWatcherModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};
    std::unique_ptr<SysCore::IO::DirectoryWatcher> m_watcher;

public:
    FileWatcherModule() = default;
    ~FileWatcherModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "FileWatcherModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "13.0.0"; }
};

#endif // FILE_WATCHER_MODULE_H
