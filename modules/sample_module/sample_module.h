#ifndef SAMPLE_MODULE_H
#define SAMPLE_MODULE_H

#include "core/imodule.h"
#include <iostream>

class SampleMemoryModule : public SysCore::Interfaces::IModule {
private:
    bool m_initialized{false};

public:
    SampleMemoryModule() = default;
    ~SampleMemoryModule() override = default;

    bool Initialize() override;
    bool Execute() override;
    void Shutdown() override;

    [[nodiscard]] const char* GetName() const noexcept override { return "SampleMemoryModule"; }
    [[nodiscard]] const char* GetVersion() const noexcept override { return "1.0.0"; }
};

#endif // SAMPLE_MODULE_H
