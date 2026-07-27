#ifndef SYSCORE_THREAD_MANAGER_H
#define SYSCORE_THREAD_MANAGER_H

#include "handle.h"
#include <windows.h>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <memory>
#include <string>
#include <stdexcept>

namespace SysCore::Threading {

    // --- RAII Kernel Event Wrapper ---
    class KernelEvent {
    private:
        HANDLE m_hEvent{nullptr};

    public:
        explicit KernelEvent(bool manualReset = false, bool initialState = false, const wchar_t* name = nullptr) {
            m_hEvent = ::CreateEventW(nullptr, manualReset ? TRUE : FALSE, initialState ? TRUE : FALSE, name);
            if (!m_hEvent) {
                throw std::runtime_error("KernelEvent: Failed to create Win32 Event object.");
            }
        }

        ~KernelEvent() {
            if (m_hEvent) {
                ::CloseHandle(m_hEvent);
                m_hEvent = nullptr;
            }
        }

        KernelEvent(const KernelEvent&) = delete;
        KernelEvent& operator=(const KernelEvent&) = delete;

        KernelEvent(KernelEvent&& other) noexcept : m_hEvent(other.m_hEvent) {
            other.m_hEvent = nullptr;
        }

        KernelEvent& operator=(KernelEvent&& other) noexcept {
            if (this != &other) {
                if (m_hEvent) ::CloseHandle(m_hEvent);
                m_hEvent = other.m_hEvent;
                other.m_hEvent = nullptr;
            }
            return *this;
        }

        void Signal() noexcept {
            if (m_hEvent) ::SetEvent(m_hEvent);
        }

        void Reset() noexcept {
            if (m_hEvent) ::ResetEvent(m_hEvent);
        }

        bool Wait(DWORD timeoutMs = INFINITE) noexcept {
            if (!m_hEvent) return false;
            return ::WaitForSingleObject(m_hEvent, timeoutMs) == WAIT_OBJECT_0;
        }

        [[nodiscard]] HANDLE Get() const noexcept { return m_hEvent; }
    };

    // --- RAII Kernel Mutex Wrapper ---
    class KernelMutex {
    private:
        HANDLE m_hMutex{nullptr};

    public:
        explicit KernelMutex(bool initialOwner = false, const wchar_t* name = nullptr) {
            m_hMutex = ::CreateMutexW(nullptr, initialOwner ? TRUE : FALSE, name);
            if (!m_hMutex) {
                throw std::runtime_error("KernelMutex: Failed to create Win32 Mutex object.");
            }
        }

        ~KernelMutex() {
            if (m_hMutex) {
                ::CloseHandle(m_hMutex);
                m_hMutex = nullptr;
            }
        }

        KernelMutex(const KernelMutex&) = delete;
        KernelMutex& operator=(const KernelMutex&) = delete;

        KernelMutex(KernelMutex&& other) noexcept : m_hMutex(other.m_hMutex) {
            other.m_hMutex = nullptr;
        }

        KernelMutex& operator=(KernelMutex&& other) noexcept {
            if (this != &other) {
                if (m_hMutex) ::CloseHandle(m_hMutex);
                m_hMutex = other.m_hMutex;
                other.m_hMutex = nullptr;
            }
            return *this;
        }

        bool Lock(DWORD timeoutMs = INFINITE) noexcept {
            if (!m_hMutex) return false;
            return ::WaitForSingleObject(m_hMutex, timeoutMs) == WAIT_OBJECT_0;
        }

        void Unlock() noexcept {
            if (m_hMutex) ::ReleaseMutex(m_hMutex);
        }

        [[nodiscard]] HANDLE Get() const noexcept { return m_hMutex; }
    };

    // --- C++20 Thread Pool Infrastructure ---
    class ThreadPool {
    private:
        std::vector<std::jthread> m_workers;
        std::queue<std::function<void()>> m_taskQueue;
        std::mutex m_queueMutex;
        std::condition_variable m_cv;
        bool m_stopRequested{false};

    public:
        explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency());
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        // Enqueue async task returning std::future
        template <typename F, typename... Args>
        auto EnqueueTask(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
            using ReturnType = std::invoke_result_t<F, Args...>;

            auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

            std::future<ReturnType> res = task->get_future();
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                if (m_stopRequested) {
                    throw std::runtime_error("ThreadPool: Cannot enqueue task on stopped ThreadPool.");
                }
                m_taskQueue.emplace([task]() { (*task)(); });
            }
            m_cv.notify_one();
            return res;
        }

        void Shutdown();
        [[nodiscard]] size_t GetWorkerCount() const noexcept { return m_workers.size(); }
        [[nodiscard]] size_t GetPendingTaskCount();
    };

    // High-level Thread Manager encapsulating thread pool & synchronization
    class ThreadManager {
    private:
        ThreadPool m_threadPool;

    public:
        explicit ThreadManager(size_t threadCount = std::thread::hardware_concurrency())
            : m_threadPool(threadCount) {}

        ~ThreadManager() = default;

        [[nodiscard]] ThreadPool& GetThreadPool() noexcept { return m_threadPool; }
        
        // Spawn standalone OS thread using RAII ThreadHandle
        static Core::ThreadHandle SpawnRawThread(LPTHREAD_START_ROUTINE threadRoutine, LPVOID param, DWORD* outThreadId = nullptr);
    };

} // namespace SysCore::Threading

#endif // SYSCORE_THREAD_MANAGER_H
