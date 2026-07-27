#ifndef SYSCORE_PIPE_MANAGER_H
#define SYSCORE_PIPE_MANAGER_H

#include "handle.h"
#include <windows.h>
#include <string>
#include <string_view>
#include <vector>

namespace SysCore::IPC {

    class NamedPipeChannel {
    private:
        std::wstring m_pipeName;
        Core::FileHandle m_hPipe;
        bool m_isServer{false};

    public:
        explicit NamedPipeChannel(std::wstring pipeName) : m_pipeName(std::move(pipeName)) {}
        ~NamedPipeChannel() = default;

        NamedPipeChannel(const NamedPipeChannel&) = delete;
        NamedPipeChannel& operator=(const NamedPipeChannel&) = delete;

        NamedPipeChannel(NamedPipeChannel&&) noexcept = default;
        NamedPipeChannel& operator=(NamedPipeChannel&&) noexcept = default;

        bool CreateServer() noexcept;
        bool ConnectClient() noexcept;

        bool Write(std::string_view message) noexcept;
        std::string Read() noexcept;

        [[nodiscard]] bool IsValid() const noexcept { return m_hPipe.IsValid(); }
        void Close() noexcept { m_hPipe.Reset(); }
    };

} // namespace SysCore::IPC

#endif // SYSCORE_PIPE_MANAGER_H
