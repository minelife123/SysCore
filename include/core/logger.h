#ifndef SYSCORE_LOGGER_H
#define SYSCORE_LOGGER_H

#include <string>
#include <string_view>
#include <mutex>
#include <fstream>
#include <memory>
#include <chrono>
#include <format>
#include <windows.h>

namespace SysCore::Logging {

    enum class LogLevel : uint8_t {
        Debug = 0,
        Info,
        Warning,
        Error
    };

    class LoggerCore {
    private:
        std::mutex m_mutex;
        std::ofstream m_fileStream;
        LogLevel m_minLogLevel{LogLevel::Debug};
        bool m_consoleOutputEnabled{true};
        bool m_fileOutputEnabled{false};

        LoggerCore() = default;
        ~LoggerCore();

        // Helper methods
        std::string GetTimestampString() const;
        const char* LogLevelToString(LogLevel level) const noexcept;
        WORD GetConsoleColor(LogLevel level) const noexcept;

    public:
        // Singleton Instance Access
        static LoggerCore& Instance() {
            static LoggerCore instance;
            return instance;
        }

        LoggerCore(const LoggerCore&) = delete;
        LoggerCore& operator=(const LoggerCore&) = delete;

        // Configuration methods
        bool Initialize(std::string_view logFilePath = "", LogLevel minLevel = LogLevel::Debug, bool enableConsole = true);
        void SetLogLevel(LogLevel level) noexcept;
        void Shutdown();

        // Main logging entry point
        void Log(LogLevel level, std::string_view message);

        // Convenient helper functions
        void Debug(std::string_view message) { Log(LogLevel::Debug, message); }
        void Info(std::string_view message) { Log(LogLevel::Info, message); }
        void Warning(std::string_view message) { Log(LogLevel::Warning, message); }
        void Error(std::string_view message) { Log(LogLevel::Error, message); }

        // Formatted log support using C++20 std::format
        template <typename... Args>
        void LogFormat(LogLevel level, std::string_view fmt, Args&&... args) {
            try {
                std::string formattedMsg = std::vformat(fmt, std::make_format_args(args...));
                Log(level, formattedMsg);
            } catch (const std::exception& ex) {
                Log(LogLevel::Error, std::string("Log format error: ") + ex.what());
            }
        }
    };

} // namespace SysCore::Logging

#endif // SYSCORE_LOGGER_H
