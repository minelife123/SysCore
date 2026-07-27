#ifndef SYSCORE_PROFILER_H
#define SYSCORE_PROFILER_H

#include <string>
#include <string_view>
#include <chrono>
#include <unordered_map>
#include <shared_mutex>
#include <vector>

namespace SysCore::Profiling {

    struct ProfileResult {
        std::string name;
        uint64_t callCount{0};
        double totalDurationMs{0.0};
        double minDurationMs{1e9};
        double maxDurationMs{0.0};
        double avgDurationMs{0.0};
    };

    class PerformanceProfiler {
    private:
        std::unordered_map<std::string, ProfileResult> m_results;
        mutable std::shared_mutex m_mutex;

        PerformanceProfiler() = default;

    public:
        static PerformanceProfiler& Instance() {
            static PerformanceProfiler instance;
            return instance;
        }

        PerformanceProfiler(const PerformanceProfiler&) = delete;
        PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;

        void RecordExecution(std::string_view scopeName, double durationMs);
        [[nodiscard]] std::vector<ProfileResult> GetResults() const;
        void LogReport() const;
        void Reset();
    };

    class ScopedTimer {
    private:
        std::string m_name;
        std::chrono::high_resolution_clock::time_point m_startTime;

    public:
        explicit ScopedTimer(std::string_view name)
            : m_name(name), m_startTime(std::chrono::high_resolution_clock::now()) {}

        ~ScopedTimer() {
            auto endTime = std::chrono::high_resolution_clock::now();
            double durationMs = std::chrono::duration<double, std::milli>(endTime - m_startTime).count();
            PerformanceProfiler::Instance().RecordExecution(m_name, durationMs);
        }
    };

} // namespace SysCore::Profiling

#define SYSCORE_PROFILE_SCOPE(name) ::SysCore::Profiling::ScopedTimer timer_##__LINE__(name)
#define SYSCORE_PROFILE_FUNCTION() SYSCORE_PROFILE_SCOPE(__FUNCTION__)

#endif // SYSCORE_PROFILER_H
