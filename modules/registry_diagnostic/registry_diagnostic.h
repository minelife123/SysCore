#ifndef REGISTRY_DIAGNOSTIC_MODULE_H
#define REGISTRY_DIAGNOSTIC_MODULE_H

#include "core/imodule.h"
#include <string>
#include <cstdint>

struct RegistryAuditMetrics {
    std::string productName;
    std::string currentBuild;
    size_t autorunEntriesCount{0};
};

class RegistryDiagnosticModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};

public:
    RegistryDiagnosticModule() = default;
    ~RegistryDiagnosticModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "RegistryDiagnosticModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "9.0.0"; }

    [[nodiscard]] RegistryAuditMetrics AuditRegistry() const noexcept;
};

#endif // REGISTRY_DIAGNOSTIC_MODULE_H
