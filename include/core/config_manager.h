#ifndef SYSCORE_CONFIG_MANAGER_H
#define SYSCORE_CONFIG_MANAGER_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <filesystem>
#include <shared_mutex>
#include <optional>

namespace SysCore::Config {

    class ConfigManager {
    private:
        std::unordered_map<std::string, std::string> m_configData;
        mutable std::shared_mutex m_mutex;
        std::filesystem::path m_configFilePath;

    public:
        ConfigManager() = default;
        explicit ConfigManager(const std::filesystem::path& filePath);
        ~ConfigManager() = default;

        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;

        // File I/O operations
        bool LoadFromFile(const std::filesystem::path& filePath);
        bool SaveToFile(const std::filesystem::path& filePath = "") const;
        void CreateDefault(const std::filesystem::path& filePath);

        // Value Accessors
        void SetString(std::string_view key, std::string_view value);
        void SetInt(std::string_view key, int value);
        void SetBool(std::string_view key, bool value);
        void SetDouble(std::string_view key, double value);

        [[nodiscard]] std::string GetString(std::string_view key, std::string_view defaultValue = "") const;
        [[nodiscard]] int GetInt(std::string_view key, int defaultValue = 0) const;
        [[nodiscard]] bool GetBool(std::string_view key, bool defaultValue = false) const;
        [[nodiscard]] double GetDouble(std::string_view key, double defaultValue = 0.0) const;

        [[nodiscard]] bool HasKey(std::string_view key) const;
        [[nodiscard]] size_t GetKeyCount() const;
    };

} // namespace SysCore::Config

#endif // SYSCORE_CONFIG_MANAGER_H
