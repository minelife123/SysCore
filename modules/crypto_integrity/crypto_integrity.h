#ifndef CRYPTO_INTEGRITY_MODULE_H
#define CRYPTO_INTEGRITY_MODULE_H

#include "core/imodule.h"
#include <string>

class CryptoIntegrityModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};

public:
    CryptoIntegrityModule() = default;
    ~CryptoIntegrityModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "CryptoIntegrityModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "6.0.0"; }
};

#endif // CRYPTO_INTEGRITY_MODULE_H
