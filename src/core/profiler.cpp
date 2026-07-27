#include "core/profiler.h"
#include "core/logger.h"
#include <algorithm>

namespace SysCore::Profiling {

    void PerformanceProfiler::RecordExecution(std::string_view scopeName, double durationMs) {
        std::unique_lock lock(m_mutex);
        std::string key(scopeName);

        auto& res = m_results[key];
        if (res.callCount == 0) {
            res.name = key;
        }

        res.callCount++;
        res.totalDurationMs += durationMs;
        res.minDurationMs = (std::min)(res.minDurationMs, durationMs);
        res.maxDurationMs = (std::max)(res.maxDurationMs, durationMs);
        res.avgDurationMs = res.totalDurationMs / static_cast<double>(res.callCount);
    }

    std::vector<ProfileResult> PerformanceProfiler::GetResults() const {
        std::shared_lock lock(m_mutex);
        std::vector<ProfileResult> list;
        list.reserve(m_results.size());

        for (const auto& [_, val] : m_results) {
            list.push_back(val);
        }
        return list;
    }

    void PerformanceProfiler::LogReport() const {
        auto results = GetResults();
        if (results.empty()) return;

        auto& logger = Logging::LoggerCore::Instance();
        logger.Info("=================================================");
        logger.Info("        Performance Profiler Summary Report      ");
        logger.Info("=================================================");

        for (const auto& res : results) {
            logger.LogFormat(Logging::LogLevel::Info,
                "Scope: {:<30} | Calls: {:<4} | Avg: {:.3f} ms | Min: {:.3f} ms | Max: {:.3f} ms | Total: {:.3f} ms",
                res.name, res.callCount, res.avgDurationMs, res.minDurationMs, res.maxDurationMs, res.totalDurationMs);
        }
        logger.Info("=================================================");
    }

    void PerformanceProfiler::Reset() {
        std::unique_lock lock(m_mutex);
        m_results.clear();
    }

} // namespace SysCore::Profiling
