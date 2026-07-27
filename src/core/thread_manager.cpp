#include "core/thread_manager.h"
#include "core/logger.h"

namespace SysCore::Threading {

    // --- ThreadPool Implementation ---

    ThreadPool::ThreadPool(size_t threadCount) {
        if (threadCount == 0) {
            threadCount = 1;
        }

        m_workers.reserve(threadCount);
        for (size_t i = 0; i < threadCount; ++i) {
            m_workers.emplace_back([this](std::stop_token stopToken) {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(m_queueMutex);
                        m_cv.wait(lock, [this, &stopToken] {
                            return m_stopRequested || stopToken.stop_requested() || !m_taskQueue.empty();
                        });

                        if ((m_stopRequested || stopToken.stop_requested()) && m_taskQueue.empty()) {
                            return;
                        }

                        if (!m_taskQueue.empty()) {
                            task = std::move(m_taskQueue.front());
                            m_taskQueue.pop();
                        }
                    }

                    if (task) {
                        try {
                            task();
                        } catch (const std::exception& ex) {
                            Logging::LoggerCore::Instance().Error(
                                std::string("ThreadPool worker caught exception: ") + ex.what());
                        } catch (...) {
                            Logging::LoggerCore::Instance().Error(
                                "ThreadPool worker caught unknown exception.");
                        }
                    }
                }
            });
        }

        Logging::LoggerCore::Instance().Info(
            "ThreadPool initialized with " + std::to_string(threadCount) + " worker threads.");
    }

    ThreadPool::~ThreadPool() {
        Shutdown();
    }

    void ThreadPool::Shutdown() {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (m_stopRequested) return;
            m_stopRequested = true;
        }
        m_cv.notify_all();

        for (auto& worker : m_workers) {
            if (worker.joinable()) {
                worker.request_stop();
            }
        }
        m_workers.clear();

        Logging::LoggerCore::Instance().Info("ThreadPool successfully shut down.");
    }

    size_t ThreadPool::GetPendingTaskCount() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return m_taskQueue.size();
    }

    // --- ThreadManager Implementation ---

    Core::ThreadHandle ThreadManager::SpawnRawThread(LPTHREAD_START_ROUTINE threadRoutine, LPVOID param, DWORD* outThreadId) {
        HANDLE hThread = ::CreateThread(nullptr, 0, threadRoutine, param, 0, outThreadId);
        if (!hThread) {
            Logging::LoggerCore::Instance().Error(
                "ThreadManager::SpawnRawThread failed to create thread. Error: " + std::to_string(::GetLastError()));
        }
        return Core::ThreadHandle(hThread);
    }

} // namespace SysCore::Threading
