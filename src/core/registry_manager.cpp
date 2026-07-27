#include "core/registry_manager.h"
#include "core/logger.h"

namespace SysCore::Registry {

    static HKEY GetHKey(RegHive hive) noexcept {
        switch (hive) {
            case RegHive::LocalMachine: return HKEY_LOCAL_MACHINE;
            case RegHive::CurrentUser:  return HKEY_CURRENT_USER;
            case RegHive::ClassesRoot:  return HKEY_CLASSES_ROOT;
            default:                    return HKEY_LOCAL_MACHINE;
        }
    }

    static std::wstring ToWString(const std::string& str) {
        if (str.empty()) return L"";
        int size = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring wstr(size, L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wstr.data(), size);
        wstr.resize(size - 1);
        return wstr;
    }

    static std::string ToString(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int size = ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(size, '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, str.data(), size, nullptr, nullptr);
        str.resize(size - 1);
        return str;
    }

    std::optional<std::string> RegistryManager::ReadString(
        RegHive hive, const std::string& subkey, const std::string& valueName) noexcept {

        HKEY hKey = nullptr;
        std::wstring wSubkey = ToWString(subkey);
        std::wstring wValueName = ToWString(valueName);

        if (::RegOpenKeyExW(GetHKey(hive), wSubkey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            return std::nullopt;
        }

        DWORD type = 0, bytesNeeded = 0;
        if (::RegQueryValueExW(hKey, wValueName.c_str(), nullptr, &type, nullptr, &bytesNeeded) != ERROR_SUCCESS ||
            (type != REG_SZ && type != REG_EXPAND_SZ)) {
            ::RegCloseKey(hKey);
            return std::nullopt;
        }

        std::wstring buffer(bytesNeeded / sizeof(wchar_t), L'\0');
        if (::RegQueryValueExW(hKey, wValueName.c_str(), nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer.data()), &bytesNeeded) == ERROR_SUCCESS) {
            ::RegCloseKey(hKey);
            return ToString(buffer.c_str());
        }

        ::RegCloseKey(hKey);
        return std::nullopt;
    }

    std::optional<uint32_t> RegistryManager::ReadDword(
        RegHive hive, const std::string& subkey, const std::string& valueName) noexcept {

        HKEY hKey = nullptr;
        std::wstring wSubkey = ToWString(subkey);
        std::wstring wValueName = ToWString(valueName);

        if (::RegOpenKeyExW(GetHKey(hive), wSubkey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            return std::nullopt;
        }

        DWORD type = 0, value = 0, dataSize = sizeof(DWORD);
        if (::RegQueryValueExW(hKey, wValueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(&value), &dataSize) == ERROR_SUCCESS &&
            type == REG_DWORD) {
            ::RegCloseKey(hKey);
            return value;
        }

        ::RegCloseKey(hKey);
        return std::nullopt;
    }

    std::vector<RegistryKeyValue> RegistryManager::EnumSubkeyValues(
        RegHive hive, const std::string& subkey) noexcept {

        std::vector<RegistryKeyValue> result;
        HKEY hKey = nullptr;
        std::wstring wSubkey = ToWString(subkey);

        if (::RegOpenKeyExW(GetHKey(hive), wSubkey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            return result;
        }

        DWORD valueCount = 0, maxValNameLen = 0, maxValLen = 0;
        ::RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            &valueCount, &maxValNameLen, &maxValLen, nullptr, nullptr);

        for (DWORD i = 0; i < valueCount; ++i) {
            std::vector<wchar_t> nameBuf(maxValNameLen + 1, L'\0');
            DWORD nameSize = maxValNameLen + 1;
            DWORD type = 0;
            std::vector<BYTE> dataBuf(maxValLen + 2, 0);
            DWORD dataSize = maxValLen + 2;

            if (::RegEnumValueW(hKey, i, nameBuf.data(), &nameSize, nullptr, &type, dataBuf.data(), &dataSize) == ERROR_SUCCESS) {
                RegistryKeyValue entry{};
                entry.name = ToString(nameBuf.data());
                entry.type = type;
                if (type == REG_SZ || type == REG_EXPAND_SZ) {
                    entry.value = ToString(reinterpret_cast<wchar_t*>(dataBuf.data()));
                } else if (type == REG_DWORD && dataSize >= sizeof(DWORD)) {
                    uint32_t val = *reinterpret_cast<uint32_t*>(dataBuf.data());
                    entry.value = std::to_string(val);
                } else {
                    entry.value = "<binary data>";
                }
                result.push_back(entry);
            }
        }

        ::RegCloseKey(hKey);
        return result;
    }

} // namespace SysCore::Registry
