#ifndef SYSCORE_ASYNC_FILE_H
#define SYSCORE_ASYNC_FILE_H

#include "handle.h"
#include <windows.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <future>

namespace SysCore::IO {

    class AsyncFileEngine {
    public:
        static std::future<bool> WriteFileAsync(const std::filesystem::path& path, std::string_view content);
        static std::future<std::string> ReadFileAsync(const std::filesystem::path& path);
    };

} // namespace SysCore::IO

#endif // SYSCORE_ASYNC_FILE_H
