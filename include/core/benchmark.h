#ifndef SYSCORE_BENCHMARK_H
#define SYSCORE_BENCHMARK_H

#include <string>
#include <string_view>
#include <functional>
#include <chrono>
#include <cstdint>

namespace SysCore::Profiling {

    struct BenchmarkResult {
        std::string name;
        size_t iterations{0};
        double totalTimeMs{0.0};
        double opsPerSecond{0.0};
        double nsPerOp{0.0};
    };

    class BenchmarkEngine {
    public:
        [[nodiscard]] static BenchmarkResult RunBenchmark(
            std::string_view name,
            size_t iterations,
            const std::function<void()>& func);
    };

} // namespace SysCore::Profiling

#endif // SYSCORE_BENCHMARK_H
