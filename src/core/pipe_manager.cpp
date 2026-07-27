#include "core/pipe_manager.h"
#include "core/logger.h"

namespace SysCore::IPC {

    bool NamedPipeChannel::CreateServer() noexcept {
        std::wstring fullPath = L"\\\\.\\pipe\\" + m_pipeName;

        HANDLE hPipe = ::CreateNamedPipeW(
            fullPath.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1, // Max instances
            4096, 4096, 0, nullptr);

        if (hPipe == INVALID_HANDLE_VALUE) {
            Logging::LoggerCore::Instance().Error("[NamedPipeChannel] Failed to create named pipe server.");
            return false;
        }

        m_hPipe = Core::FileHandle(hPipe);
        m_isServer = true;
        return true;
    }

    bool NamedPipeChannel::ConnectClient() noexcept {
        std::wstring fullPath = L"\\\\.\\pipe\\" + m_pipeName;

        HANDLE hPipe = ::CreateFileW(
            fullPath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, 0, nullptr);

        if (hPipe == INVALID_HANDLE_VALUE) {
            Logging::LoggerCore::Instance().Error("[NamedPipeChannel] Failed to connect to named pipe server.");
            return false;
        }

        m_hPipe = Core::FileHandle(hPipe);
        m_isServer = false;
        return true;
    }

    bool NamedPipeChannel::Write(std::string_view message) noexcept {
        if (!m_hPipe.IsValid()) return false;

        DWORD bytesWritten = 0;
        BOOL ok = ::WriteFile(m_hPipe.Get(), message.data(), static_cast<DWORD>(message.size()), &bytesWritten, nullptr);
        return ok != FALSE && bytesWritten == message.size();
    }

    std::string NamedPipeChannel::Read() noexcept {
        if (!m_hPipe.IsValid()) return "";

        char buffer[1024]{0};
        DWORD bytesRead = 0;
        BOOL ok = ::ReadFile(m_hPipe.Get(), buffer, sizeof(buffer) - 1, &bytesRead, nullptr);
        if (ok && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            return std::string(buffer, bytesRead);
        }
        return "";
    }

} // namespace SysCore::IPC
