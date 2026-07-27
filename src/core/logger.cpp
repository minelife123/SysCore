#include "core/logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace SysCore::Logging {

    LoggerCore::~LoggerCore() {
        Shutdown();
    }

    bool LoggerCore::Initialize(std::string_view logFilePath, LogLevel minLevel, bool enableConsole) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_minLogLevel = minLevel;
        m_consoleOutputEnabled = enableConsole;

        if (!logFilePath.empty()) {
            m_fileStream.open(std::string(logFilePath), std::ios::out | std::ios::app);
            if (m_fileStream.is_open()) {
                m_fileOutputEnabled = true;
            } else {
                std::cerr << "[LoggerCore] Failed to open log file: " << logFilePath << std::endl;
                m_fileOutputEnabled = false;
                return false;
            }
        }
        return true;
    }

    void LoggerCore::SetLogLevel(LogLevel level) noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_minLogLevel = level;
    }

    void LoggerCore::Shutdown() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_fileStream.is_open()) {
            m_fileStream.flush();
            m_fileStream.close();
        }
        m_fileOutputEnabled = false;
    }

    std::string LoggerCore::GetTimestampString() const {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::tm tm_buf{};
        localtime_s(&tm_buf, &in_time_t);

        std::ostringstream ss;
        ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
           << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    const char* LoggerCore::LogLevelToString(LogLevel level) const noexcept {
        switch (level) {
            case LogLevel::Debug:   return "DEBUG";
            case LogLevel::Info:    return "INFO ";
            case LogLevel::Warning: return "WARN ";
            case LogLevel::Error:   return "ERROR";
            default:                return "UNKNW";
        }
    }

    WORD LoggerCore::GetConsoleColor(LogLevel level) const noexcept {
        switch (level) {
            case LogLevel::Debug:   return FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN; // Cyan
            case LogLevel::Info:    return FOREGROUND_INTENSITY | FOREGROUND_GREEN;                   // Bright Green
            case LogLevel::Warning: return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN;  // Bright Yellow
            case LogLevel::Error:   return FOREGROUND_INTENSITY | FOREGROUND_RED;                    // Bright Red
            default:                return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;       // White
        }
    }

    void LoggerCore::Log(LogLevel level, std::string_view message) {
        if (level < m_minLogLevel) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        std::string timestamp = GetTimestampString();
        const char* levelStr = LogLevelToString(level);

        std::string formattedEntry = std::string("[") + timestamp + "] [" + levelStr + "] " + std::string(message);

        // 1. Console Output
        if (m_consoleOutputEnabled) {
            HANDLE hConsole = ::GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO origBufferInfo{};

            if (hConsole != INVALID_HANDLE_VALUE && ::GetConsoleScreenBufferInfo(hConsole, &origBufferInfo)) {
                ::SetConsoleTextAttribute(hConsole, GetConsoleColor(level));
                if (level == LogLevel::Error) {
                    std::cerr << formattedEntry << std::endl;
                } else {
                    std::cout << formattedEntry << std::endl;
                }
                ::SetConsoleTextAttribute(hConsole, origBufferInfo.wAttributes);
            } else {
                std::cout << formattedEntry << std::endl;
            }
        }

        // 2. File Output
        if (m_fileOutputEnabled && m_fileStream.is_open()) {
            m_fileStream << formattedEntry << "\n";
            m_fileStream.flush();
        }
    }

} // namespace SysCore::Logging
