#ifndef SYSCORE_FILE_WATCHER_H
#define SYSCORE_FILE_WATCHER_H

#include "handle.h"
#include <windows.h>
#include <filesystem>
#include <string>
#include <functional>
#include <thread>
#include <atomic>

namespace SysCore::IO {

    enum class FileAction {
        Added = 1,
        Removed = 2,
        Modified = 3,
        RenamedOldName = 4,
        RenamedNewName = 5
    };

    struct FileChangeEvent {
        std::filesystem::path path;
        FileAction action;
    };

    using FileChangeCallback = std::function<void(const FileChangeEvent&)>;

    class DirectoryWatcher {
    private:
        std::filesystem::path m_watchPath;
        Core::FileHandle m_hDirectory;
        std::atomic<bool> m_running{false};
        std::jthread m_watchThread;
        FileChangeCallback m_callback;

        void WatchLoop();

    public:
        explicit DirectoryWatcher(std::filesystem::path watchPath) : m_watchPath(std::move(watchPath)) {}
        ~DirectoryWatcher() { Stop(); }

        bool Start(FileChangeCallback callback) noexcept;
        void Stop() noexcept;

        [[nodiscard]] bool IsRunning() const noexcept { return m_running.load(); }
    };

} // namespace SysCore::IO

#endif // SYSCORE_FILE_WATCHER_H
