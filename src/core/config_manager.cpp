#include "core/config_manager.h"
#include "core/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace SysCore::Config {

    static std::string Trim(std::string_view str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return std::string(str.substr(first, (last - first + 1)));
    }

    ConfigManager::ConfigManager(const std::filesystem::path& filePath) {
        if (!LoadFromFile(filePath)) {
            CreateDefault(filePath);
        }
    }

    bool ConfigManager::LoadFromFile(const std::filesystem::path& filePath) {
        std::unique_lock lock(m_mutex);
        m_configFilePath = filePath;
        m_configData.clear();

        if (!std::filesystem::exists(filePath)) {
            return false;
        }

        std::ifstream file(filePath);
        if (!file.is_open()) {
            Logging::LoggerCore::Instance().Warning("ConfigManager: Failed to open config file: " + filePath.string());
            return false;
        }

        std::string line;
        std::string currentSection;

        while (std::getline(file, line)) {
            std::string trimmed = Trim(line);
            if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
                continue; // Skip comments and empty lines
            }

            if (trimmed.front() == '[' && trimmed.back() == ']') {
                currentSection = Trim(trimmed.substr(1, trimmed.size() - 2));
                continue;
            }

            auto delimiterPos = trimmed.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = Trim(trimmed.substr(0, delimiterPos));
                std::string value = Trim(trimmed.substr(delimiterPos + 1));

                if (!currentSection.empty()) {
                    key = currentSection + "." + key;
                }

                m_configData[key] = value;
            }
        }

        Logging::LoggerCore::Instance().Info(
            "ConfigManager: Successfully loaded " + std::to_string(m_configData.size()) + " entries from " + filePath.string());
        return true;
    }

    bool ConfigManager::SaveToFile(const std::filesystem::path& filePath) const {
        std::shared_lock lock(m_mutex);
        std::filesystem::path targetPath = filePath.empty() ? m_configFilePath : filePath;

        if (targetPath.empty()) {
            return false;
        }

        std::ofstream file(targetPath);
        if (!file.is_open()) {
            Logging::LoggerCore::Instance().Error("ConfigManager: Failed to write config file: " + targetPath.string());
            return false;
        }

        file << "# SysCore Windows Framework Configuration File\n\n";
        for (const auto& [key, value] : m_configData) {
            file << key << " = " << value << "\n";
        }

        return true;
    }

    void ConfigManager::CreateDefault(const std::filesystem::path& filePath) {
        {
            std::unique_lock lock(m_mutex);
            m_configFilePath = filePath;
            m_configData.clear();
            m_configData["app_name"] = "SysCore Windows x64 Framework";
            m_configData["log_level"] = "0"; // Debug
            m_configData["worker_threads"] = "0"; // Auto (hardware concurrency)
            m_configData["enable_console_logging"] = "true";
            m_configData["modules_directory"] = "build/bin/modules/Debug";
            m_configData["ipc_channel_name"] = "SysCore_SharedIPCChannel";
            m_configData["auto_reload_interval_ms"] = "1000";
        }
        SaveToFile(filePath);
        Logging::LoggerCore::Instance().Info("ConfigManager: Created default configuration file at " + filePath.string());
    }

    void ConfigManager::SetString(std::string_view key, std::string_view value) {
        std::unique_lock lock(m_mutex);
        m_configData[std::string(key)] = std::string(value);
    }

    void ConfigManager::SetInt(std::string_view key, int value) {
        SetString(key, std::to_string(value));
    }

    void ConfigManager::SetBool(std::string_view key, bool value) {
        SetString(key, value ? "true" : "false");
    }

    void ConfigManager::SetDouble(std::string_view key, double value) {
        SetString(key, std::to_string(value));
    }

    std::string ConfigManager::GetString(std::string_view key, std::string_view defaultValue) const {
        std::shared_lock lock(m_mutex);
        auto it = m_configData.find(std::string(key));
        if (it != m_configData.end()) {
            return it->second;
        }
        return std::string(defaultValue);
    }

    int ConfigManager::GetInt(std::string_view key, int defaultValue) const {
        std::string valStr = GetString(key);
        if (valStr.empty()) return defaultValue;
        try {
            return std::stoi(valStr);
        } catch (...) {
            return defaultValue;
        }
    }

    bool ConfigManager::GetBool(std::string_view key, bool defaultValue) const {
        std::string valStr = GetString(key);
        if (valStr.empty()) return defaultValue;
        std::string lowerVal = Trim(valStr);
        std::transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return (lowerVal == "true" || lowerVal == "1" || lowerVal == "yes");
    }

    double ConfigManager::GetDouble(std::string_view key, double defaultValue) const {
        std::string valStr = GetString(key);
        if (valStr.empty()) return defaultValue;
        try {
            return std::stod(valStr);
        } catch (...) {
            return defaultValue;
        }
    }

    bool ConfigManager::HasKey(std::string_view key) const {
        std::shared_lock lock(m_mutex);
        return m_configData.contains(std::string(key));
    }

    size_t ConfigManager::GetKeyCount() const {
        std::shared_lock lock(m_mutex);
        return m_configData.size();
    }

} // namespace SysCore::Config
