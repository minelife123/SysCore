#include "sample_module.h"
#include "core/logger.h"
#include "core/resource_manager.h"
#include <iostream>

bool SampleMemoryModule::Initialize() {
    SysCore::Logging::LoggerCore::Instance().Info("[SampleMemoryModule] Initializing dynamic memory module...");
    m_initialized = true;
    return true;
}

bool SampleMemoryModule::Execute() {
    if (!m_initialized) return false;

    auto& logger = SysCore::Logging::LoggerCore::Instance();
    logger.Info("[SampleMemoryModule] Executing dynamic system diagnostic task...");

    // Allocate heap memory using core RAII wrapper
    SysCore::Resources::ScopedHeapMem heapMem(4096);
    if (heapMem.IsValid()) {
        logger.Info("[SampleMemoryModule] Allocated 4KB Heap block inside dynamic module.");
        char* buf = heapMem.As<char>();
        std::snprintf(buf, 4096, "Dynamic Module Memory Test String");
        logger.Info(std::string("[SampleMemoryModule] Buffer contents: ") + buf);
    }

    return true;
}

void SampleMemoryModule::Shutdown() {
    if (m_initialized) {
        SysCore::Logging::LoggerCore::Instance().Info("[SampleMemoryModule] Shutting down dynamic module.");
        m_initialized = false;
    }
}

// Export C Factory functions
SYSCORE_MODULE_API SysCore::Interfaces::IModule* CreateModule() {
    return new SampleMemoryModule();
}

SYSCORE_MODULE_API void DestroyModule(SysCore::Interfaces::IModule* modulePtr) {
    delete modulePtr;
}
