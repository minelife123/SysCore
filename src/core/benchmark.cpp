#include "core/benchmark.h"
#include "core/logger.h"

namespace SysCore::Profiling {

    BenchmarkResult BenchmarkEngine::RunBenchmark(
        std::string_view name,
        size_t iterations,
        const std::function<void()>& func) {

        BenchmarkResult result{};
        result.name = std::string(name);
        result.iterations = iterations;

        if (iterations == 0 || !func) return result;

        // Warmup run
        func();

        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < iterations; ++i) {
            func();
        }
        auto end = std::chrono::high_resolution_clock::now();

        double totalSec = std::chrono::duration<double>(end - start).count();
        result.totalTimeMs = totalSec * 1000.0;
        result.opsPerSecond = static_cast<double>(iterations) / totalSec;
        result.nsPerOp = (totalSec * 1e9) / static_cast<double>(iterations);

        Logging::LoggerCore::Instance().LogFormat(Logging::LogLevel::Info,
            "[BenchmarkEngine] {:<25} | {:<8} ops | Total: {:.3f} ms | {:.2f} ops/sec | {:.1f} ns/op",
            result.name, result.iterations, result.totalTimeMs, result.opsPerSecond, result.nsPerOp);

        return result;
    }

} // namespace SysCore::Profiling
