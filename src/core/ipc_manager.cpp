#include "core/ipc_manager.h"
#include "core/logger.h"

namespace SysCore::IPC {

    // Helper functions for reading telemetry packets from SharedMemoryChannel
    bool ReadSharedTelemetry(const SharedMemoryChannel& channel, SharedTelemetryHeader& outHeader) {
        if (!channel.IsValid()) return false;
        auto ptr = channel.GetData<SharedTelemetryHeader>();
        if (!ptr) return false;

        outHeader.sequenceNumber.store(ptr->sequenceNumber.load(std::memory_order_relaxed));
        outHeader.senderProcessId = ptr->senderProcessId;
        std::memcpy(outHeader.channelName, ptr->channelName, sizeof(outHeader.channelName));
        std::memcpy(outHeader.messagePayload, ptr->messagePayload, sizeof(outHeader.messagePayload));
        return true;
    }

} // namespace SysCore::IPC
