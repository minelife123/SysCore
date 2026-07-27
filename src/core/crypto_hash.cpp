#include "core/crypto_hash.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace SysCore::Crypto {

    static std::string ToHex(const uint8_t* hash, DWORD hashLen) {
        std::stringstream ss;
        for (DWORD i = 0; i < hashLen; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return ss.str();
    }

    std::string CryptoHashManager::ComputeSha256(const uint8_t* data, size_t size) noexcept {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_HASH_HANDLE hHash = nullptr;

        if (::BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
            return "";
        }

        DWORD cbHashObject = 0, cbData = 0;
        ::BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&cbHashObject), sizeof(DWORD), &cbData, 0);

        DWORD cbHash = 0;
        ::BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&cbHash), sizeof(DWORD), &cbData, 0);

        std::vector<BYTE> hashObject(cbHashObject);
        std::vector<BYTE> hashResult(cbHash);

        if (::BCryptCreateHash(hAlg, &hHash, hashObject.data(), cbHashObject, nullptr, 0, 0) != 0) {
            ::BCryptCloseAlgorithmProvider(hAlg, 0);
            return "";
        }

        ::BCryptHashData(hHash, const_cast<PUCHAR>(data), static_cast<ULONG>(size), 0);
        ::BCryptFinishHash(hHash, hashResult.data(), cbHash, 0);

        ::BCryptDestroyHash(hHash);
        ::BCryptCloseAlgorithmProvider(hAlg, 0);

        return ToHex(hashResult.data(), cbHash);
    }

    std::string CryptoHashManager::ComputeSha256(std::string_view text) noexcept {
        return ComputeSha256(reinterpret_cast<const uint8_t*>(text.data()), text.size());
    }

    std::string CryptoHashManager::ComputeFileSha256(const std::filesystem::path& filePath) noexcept {
        if (!std::filesystem::exists(filePath)) return "";
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return "";

        std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return ComputeSha256(buffer.data(), buffer.size());
    }

} // namespace SysCore::Crypto
