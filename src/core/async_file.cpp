#include "core/async_file.h"
#include "core/logger.h"
#include <vector>

namespace SysCore::IO {

    std::future<bool> AsyncFileEngine::WriteFileAsync(const std::filesystem::path& path, std::string_view content) {
        std::string text(content);
        return std::async(std::launch::async, [path, text]() -> bool {
            Core::FileHandle hFile(::CreateFileW(
                path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr));

            if (!hFile.IsValid()) return false;

            OVERLAPPED ov{};
            ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
            Core::FileHandle hEvent(ov.hEvent);

            DWORD bytesWritten = 0;
            BOOL ok = ::WriteFile(hFile.Get(), text.data(), static_cast<DWORD>(text.size()), &bytesWritten, &ov);
            if (!ok && ::GetLastError() == ERROR_IO_PENDING) {
                ::GetOverlappedResult(hFile.Get(), &ov, &bytesWritten, TRUE);
                ok = TRUE;
            }

            return ok != FALSE;
        });
    }

    std::future<std::string> AsyncFileEngine::ReadFileAsync(const std::filesystem::path& path) {
        return std::async(std::launch::async, [path]() -> std::string {
            Core::FileHandle hFile(::CreateFileW(
                path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr));

            if (!hFile.IsValid()) return "";

            DWORD fileSize = ::GetFileSize(hFile.Get(), nullptr);
            if (fileSize == INVALID_FILE_SIZE || fileSize == 0) return "";

            std::string buffer(fileSize, '\0');
            OVERLAPPED ov{};
            ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
            Core::FileHandle hEvent(ov.hEvent);

            DWORD bytesRead = 0;
            BOOL ok = ::ReadFile(hFile.Get(), buffer.data(), fileSize, &bytesRead, &ov);
            if (!ok && ::GetLastError() == ERROR_IO_PENDING) {
                ::GetOverlappedResult(hFile.Get(), &ov, &bytesRead, TRUE);
                ok = TRUE;
            }

            if (ok) {
                buffer.resize(bytesRead);
                return buffer;
            }
            return "";
        });
    }

} // namespace SysCore::IO
