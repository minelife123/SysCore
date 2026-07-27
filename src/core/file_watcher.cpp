#include "core/file_watcher.h"
#include "core/logger.h"
#include <vector>

namespace SysCore::IO {

    bool DirectoryWatcher::Start(FileChangeCallback callback) noexcept {
        if (m_running.load()) return true;

        if (!std::filesystem::exists(m_watchPath)) {
            std::filesystem::create_directories(m_watchPath);
        }

        HANDLE hDir = ::CreateFileW(
            m_watchPath.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            nullptr);

        if (hDir == INVALID_HANDLE_VALUE) {
            Logging::LoggerCore::Instance().Error("[DirectoryWatcher] Failed to open handle for directory monitoring.");
            return false;
        }

        m_hDirectory = Core::FileHandle(hDir);
        m_callback = std::move(callback);
        m_running.store(true);

        m_watchThread = std::jthread([this]() { WatchLoop(); });
        Logging::LoggerCore::Instance().Info("[DirectoryWatcher] Started monitoring directory: " + m_watchPath.string());
        return true;
    }

    void DirectoryWatcher::Stop() noexcept {
        if (m_running.exchange(false)) {
            if (m_hDirectory.IsValid()) {
                ::CancelIoEx(m_hDirectory.Get(), nullptr);
            }
            m_hDirectory.Reset();
            if (m_watchThread.joinable()) {
                m_watchThread.request_stop();
            }
            Logging::LoggerCore::Instance().Info("[DirectoryWatcher] Stopped directory monitoring.");
        }
    }

    void DirectoryWatcher::WatchLoop() {
        BYTE buffer[4096];
        DWORD bytesReturned = 0;

        while (m_running.load() && m_hDirectory.IsValid()) {
            BOOL ok = ::ReadDirectoryChangesW(
                m_hDirectory.Get(),
                buffer,
                sizeof(buffer),
                TRUE, // Watch subtree
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                &bytesReturned,
                nullptr,
                nullptr);

            if (!ok || bytesReturned == 0 || !m_running.load()) break;

            BYTE* ptr = buffer;
            while (ptr < buffer + bytesReturned) {
                auto pInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(ptr);
                std::wstring wFilename(pInfo->FileName, pInfo->FileNameLength / sizeof(wchar_t));
                std::filesystem::path fullPath = m_watchPath / wFilename;

                FileChangeEvent evt{};
                fullPath = fullPath.lexically_normal();
                evt.path = fullPath;
                evt.action = static_cast<FileAction>(pInfo->Action);

                if (m_callback) {
                    m_callback(evt);
                }

                if (pInfo->NextEntryOffset == 0) break;
                ptr += pInfo->NextEntryOffset;
            }
        }
    }

} // namespace SysCore::IO
