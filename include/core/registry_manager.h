#ifndef SYSCORE_REGISTRY_MANAGER_H
#define SYSCORE_REGISTRY_MANAGER_H

#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace SysCore::Registry {

    enum class RegHive {
        LocalMachine = 1,
        CurrentUser = 2,
        ClassesRoot = 3
    };

    struct RegistryKeyValue {
        std::string name;
        std::string value;
        DWORD type{REG_SZ};
    };

    class RegistryManager {
    public:
        [[nodiscard]] static std::optional<std::string> ReadString(
            RegHive hive, const std::string& subkey, const std::string& valueName) noexcept;

        [[nodiscard]] static std::optional<uint32_t> ReadDword(
            RegHive hive, const std::string& subkey, const std::string& valueName) noexcept;

        [[nodiscard]] static std::vector<RegistryKeyValue> EnumSubkeyValues(
            RegHive hive, const std::string& subkey) noexcept;
    };

} // namespace SysCore::Registry

#endif // SYSCORE_REGISTRY_MANAGER_H
