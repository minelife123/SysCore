#ifndef PIPE_DIAGNOSTIC_MODULE_H
#define PIPE_DIAGNOSTIC_MODULE_H

#include "core/imodule.h"
#include <string>

class PipeDiagnosticModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};

public:
    PipeDiagnosticModule() = default;
    ~PipeDiagnosticModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "PipeDiagnosticModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "10.0.0"; }
};

#endif // PIPE_DIAGNOSTIC_MODULE_H
