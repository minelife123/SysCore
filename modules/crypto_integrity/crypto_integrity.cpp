#include "crypto_integrity.h"
#include "core/logger.h"
#include "core/crypto_hash.h"
#include <filesystem>

bool CryptoIntegrityModule::Initialize() {
    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[CryptoIntegrityModule] Initializing SHA-256 integrity verification module...");
    m_initialized = true;
    return true;
}

bool CryptoIntegrityModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();
    std::string testString = "SysCore C++20 High Performance Engine Payload";
    std::string hash = SysCore::Crypto::CryptoHashManager::ComputeSha256(testString);

    logger.LogFormat(SysCore::Logging::LogLevel::Info,
        "[CryptoIntegrityModule] SHA-256 Engine Ready -> Test Digest: {}", hash);

    return true;
}

void CryptoIntegrityModule::Shutdown() {
    if (m_initialized) {
        SysCore::Logging::LoggerCore::Instance().Info("[CryptoIntegrityModule] Shutting down integrity module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new CryptoIntegrityModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
