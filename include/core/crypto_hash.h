#ifndef SYSCORE_CRYPTO_HASH_H
#define SYSCORE_CRYPTO_HASH_H

#include <windows.h>
#include <bcrypt.h>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

#pragma comment(lib, "bcrypt.lib")

namespace SysCore::Crypto {

    class CryptoHashManager {
    public:
        [[nodiscard]] static std::string ComputeSha256(const uint8_t* data, size_t size) noexcept;
        [[nodiscard]] static std::string ComputeSha256(std::string_view text) noexcept;
        [[nodiscard]] static std::string ComputeFileSha256(const std::filesystem::path& filePath) noexcept;
    };

} // namespace SysCore::Crypto

#endif // SYSCORE_CRYPTO_HASH_H
