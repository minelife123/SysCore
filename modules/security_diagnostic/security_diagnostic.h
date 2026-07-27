#ifndef SECURITY_DIAGNOSTIC_MODULE_H
#define SECURITY_DIAGNOSTIC_MODULE_H

#include "core/imodule.h"
#include <string>
#include <cstdint>

class SecurityDiagnosticModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};

public:
    SecurityDiagnosticModule() = default;
    ~SecurityDiagnosticModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "SecurityDiagnosticModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "12.0.0"; }
};

#endif // SECURITY_DIAGNOSTIC_MODULE_H
