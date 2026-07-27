#ifndef SYSCORE_IPC_MANAGER_H
#define SYSCORE_IPC_MANAGER_H

#include "handle.h"
#include "thread_manager.h"
#include <windows.h>
#include <string>
#include <string_view>
#include <memory>
#include <stdexcept>
#include <atomic>
#include <cstring>

namespace SysCore::IPC {

    // Telemetry header layout stored inside Shared Memory buffer
    struct SharedTelemetryHeader {
        std::atomic<uint32_t> sequenceNumber{0};
        uint32_t senderProcessId{0};
        char channelName[64]{0};
        char messagePayload[512]{0};
    };

    // RAII Shared Memory IPC Channel
    class SharedMemoryChannel {
    private:
        HANDLE m_hMapFile{nullptr};
        void* m_pBuf{nullptr};
        size_t m_size{0};
        std::wstring m_channelName;
        bool m_isOwner{false};

    public:
        // Constructor for creating new shared memory or opening existing mapping
        SharedMemoryChannel(std::wstring_view name, size_t size, bool createNew = true)
            : m_size(size), m_channelName(name), m_isOwner(createNew) {
            
            if (createNew) {
                m_hMapFile = ::CreateFileMappingW(
                    INVALID_HANDLE_VALUE,
                    nullptr,
                    PAGE_READWRITE,
                    0,
                    static_cast<DWORD>(size),
                    m_channelName.c_str()
                );

                if (!m_hMapFile) {
                    throw std::runtime_error("SharedMemoryChannel: Failed to create File Mapping object.");
                }
            } else {
                m_hMapFile = ::OpenFileMappingW(
                    FILE_MAP_ALL_ACCESS,
                    FALSE,
                    m_channelName.c_str()
                );

                if (!m_hMapFile) {
                    throw std::runtime_error("SharedMemoryChannel: Failed to open existing File Mapping object.");
                }
            }

            m_pBuf = ::MapViewOfFile(
                m_hMapFile,
                FILE_MAP_ALL_ACCESS,
                0,
                0,
                size
            );

            if (!m_pBuf) {
                ::CloseHandle(m_hMapFile);
                m_hMapFile = nullptr;
                throw std::runtime_error("SharedMemoryChannel: Failed to map view of file.");
            }
        }

        ~SharedMemoryChannel() {
            Close();
        }

        // Non-copyable
        SharedMemoryChannel(const SharedMemoryChannel&) = delete;
        SharedMemoryChannel& operator=(const SharedMemoryChannel&) = delete;

        // Move constructor & assignment
        SharedMemoryChannel(SharedMemoryChannel&& other) noexcept
            : m_hMapFile(other.m_hMapFile), m_pBuf(other.m_pBuf), m_size(other.m_size),
              m_channelName(std::move(other.m_channelName)), m_isOwner(other.m_isOwner) {
            other.m_hMapFile = nullptr;
            other.m_pBuf = nullptr;
            other.m_size = 0;
        }

        SharedMemoryChannel& operator=(SharedMemoryChannel&& other) noexcept {
            if (this != &other) {
                Close();
                m_hMapFile = other.m_hMapFile;
                m_pBuf = other.m_pBuf;
                m_size = other.m_size;
                m_channelName = std::move(other.m_channelName);
                m_isOwner = other.m_isOwner;

                other.m_hMapFile = nullptr;
                other.m_pBuf = nullptr;
                other.m_size = 0;
            }
            return *this;
        }

        void Close() noexcept {
            if (m_pBuf) {
                ::UnmapViewOfFile(m_pBuf);
                m_pBuf = nullptr;
            }
            if (m_hMapFile) {
                ::CloseHandle(m_hMapFile);
                m_hMapFile = nullptr;
            }
            m_size = 0;
        }

        template <typename T>
        [[nodiscard]] T* GetData() const noexcept {
            return static_cast<T*>(m_pBuf);
        }

        [[nodiscard]] size_t GetSize() const noexcept { return m_size; }
        [[nodiscard]] bool IsValid() const noexcept { return m_pBuf != nullptr; }
    };

    constexpr size_t IPC_RING_BUFFER_CAPACITY = 16;

    // Shared Memory Ring Buffer Header for non-overwriting queue telemetry
    struct SharedRingBufferHeader {
        std::atomic<uint32_t> writeIndex{0};
        std::atomic<uint32_t> readIndex{0};
        SharedTelemetryHeader slots[IPC_RING_BUFFER_CAPACITY];
    };

    // Manager for creating, managing, and sending IPC telemetry messages
    class IpcManager {
    private:
        std::unique_ptr<SharedMemoryChannel> m_channel;
        Threading::KernelEvent m_dataReadyEvent;
        uint32_t m_sequenceCounter{0};

    public:
        IpcManager(std::wstring_view channelName = L"SysCore_SharedIPCChannel", size_t bufferSize = sizeof(SharedRingBufferHeader))
            : m_dataReadyEvent(false, false, (std::wstring(channelName) + L"_ReadyEvent").c_str()) {
            m_channel = std::make_unique<SharedMemoryChannel>(channelName, bufferSize, true);
        }

        ~IpcManager() = default;

        bool SendTelemetry(std::string_view message) {
            return PushRingBufferMessage(message);
        }

        // Push message onto the Ring Buffer queue
        bool PushRingBufferMessage(std::string_view message) {
            if (!m_channel || !m_channel->IsValid()) return false;

            auto ring = m_channel->GetData<SharedRingBufferHeader>();
            if (!ring) return false;

            uint32_t currentWrite = ring->writeIndex.load(std::memory_order_relaxed);
            uint32_t currentRead = ring->readIndex.load(std::memory_order_relaxed);

            // Check if ring buffer is full
            if ((currentWrite - currentRead) >= IPC_RING_BUFFER_CAPACITY) {
                return false; // Buffer full
            }

            uint32_t slotIdx = currentWrite % IPC_RING_BUFFER_CAPACITY;
            auto& slot = ring->slots[slotIdx];

            slot.sequenceNumber.store(++m_sequenceCounter, std::memory_order_relaxed);
            slot.senderProcessId = ::GetCurrentProcessId();
            strncpy_s(slot.messagePayload, sizeof(slot.messagePayload), message.data(), _TRUNCATE);

            ring->writeIndex.fetch_add(1, std::memory_order_release);
            m_dataReadyEvent.Signal();
            return true;
        }

        // Pop message from the Ring Buffer queue
        bool PopRingBufferMessage(SharedTelemetryHeader& outHeader) {
            if (!m_channel || !m_channel->IsValid()) return false;

            auto ring = m_channel->GetData<SharedRingBufferHeader>();
            if (!ring) return false;

            uint32_t currentRead = ring->readIndex.load(std::memory_order_relaxed);
            uint32_t currentWrite = ring->writeIndex.load(std::memory_order_acquire);

            if (currentRead == currentWrite) {
                return false; // Buffer empty
            }

            uint32_t slotIdx = currentRead % IPC_RING_BUFFER_CAPACITY;
            const auto& slot = ring->slots[slotIdx];

            outHeader.sequenceNumber.store(slot.sequenceNumber.load(std::memory_order_relaxed));
            outHeader.senderProcessId = slot.senderProcessId;
            std::memcpy(outHeader.channelName, slot.channelName, sizeof(outHeader.channelName));
            std::memcpy(outHeader.messagePayload, slot.messagePayload, sizeof(outHeader.messagePayload));

            ring->readIndex.fetch_add(1, std::memory_order_release);
            return true;
        }

        [[nodiscard]] SharedMemoryChannel& GetChannel() noexcept { return *m_channel; }
        [[nodiscard]] Threading::KernelEvent& GetDataReadyEvent() noexcept { return m_dataReadyEvent; }
    };

    // Helper functions for reading telemetry packets from SharedMemoryChannel
    bool ReadSharedTelemetry(const SharedMemoryChannel& channel, SharedTelemetryHeader& outHeader);

} // namespace SysCore::IPC

#endif // SYSCORE_IPC_MANAGER_H
