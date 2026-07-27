#include "core/lua_engine.h"
#include "core/hardware_info.h"
#include "core/benchmark.h"
#include <windows.h>
#include <tlhelp32.h>
#include <chrono>

namespace SysCore::Scripting {

    static DWORD GetActiveProcessCount()
    {
        DWORD count = 0;
        HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32 pe;
            pe.dwSize = sizeof(pe);
            if (::Process32First(hSnap, &pe))
            {
                do { count++; } while (::Process32Next(hSnap, &pe));
            }
            ::CloseHandle(hSnap);
        }
        return count;
    }

    ScriptExecutionResult LuaScriptEngine::ExecuteScript(std::string_view scriptSource) noexcept
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        ScriptExecutionResult res;
        std::ostringstream logStream;

        try
        {
            std::string code(scriptSource);

            // Simple string parsing for SysCore script API
            size_t pos = 0;
            while ((pos = code.find("SysCore.Log(\"", pos)) != std::string::npos)
            {
                pos += 13;
                size_t endPos = code.find("\")", pos);
                if (endPos != std::string::npos)
                {
                    std::string msg = code.substr(pos, endPos - pos);
                    logStream << "[LUA LOG] " << msg << "\n";
                    pos = endPos + 2;
                }
                else break;
            }

            // Parse SysCore.GetMemoryLoad()
            if (code.find("SysCore.GetMemoryLoad()") != std::string::npos)
            {
                MEMORYSTATUSEX mem{};
                mem.dwLength = sizeof(mem);
                ::GlobalMemoryStatusEx(&mem);
                logStream << "[LUA EVAL] Memory Load: " << mem.dwMemoryLoad << "%\n";
                logStream << "[LUA EVAL] Available Physical Memory: " 
                          << (mem.ullAvailPhys / (1024 * 1024)) << " MB\n";
            }

            // Parse SysCore.GetProcessCount()
            if (code.find("SysCore.GetProcessCount()") != std::string::npos)
            {
                DWORD procCount = GetActiveProcessCount();
                logStream << "[LUA EVAL] Active System Processes Count: " << procCount << "\n";
            }

            // Parse SysCore.RunBenchmark()
            if (code.find("SysCore.RunBenchmark()") != std::string::npos)
            {
                auto bench = SysCore::Profiling::BenchmarkEngine::RunBenchmark("Lua_Benchmark", 10000, []() {
                    volatile int sum = 0;
                    for (int i = 0; i < 10000; ++i) sum += i;
                });
                logStream << "[LUA BENCHMARK] Executed 10,000 iterations in " 
                          << bench.nsPerOp << " ns/op (" << bench.opsPerSecond << " ops/sec)\n";
            }

            res.success = true;
            res.outputLog = logStream.str();
            if (res.outputLog.empty())
            {
                res.outputLog = "[LUA SCRIPT] Script executed successfully with 0 output lines.\n";
            }
        }
        catch (const std::exception& ex)
        {
            res.success = false;
            res.errorMessage = ex.what();
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        res.executionTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        return res;
    }

    std::vector<std::pair<std::string, std::string>> LuaScriptEngine::GetPresetTemplates() noexcept
    {
        return {
            {
                "Memory & System Audit Script",
                "-- SysCore Diagnostic Lua Script\n"
                "SysCore.Log(\"Initializing System Audit...\")\n"
                "SysCore.GetMemoryLoad()\n"
                "SysCore.GetProcessCount()\n"
                "SysCore.Log(\"Lua Audit Completed Successfully.\")\n"
            },
            {
                "Micro-Benchmark Script",
                "-- SysCore Performance Micro-Benchmark Script\n"
                "SysCore.Log(\"Triggering High-Precision Nanosecond Benchmark...\")\n"
                "SysCore.RunBenchmark()\n"
                "SysCore.Log(\"Benchmark Execution Done.\")\n"
            }
        };
    }

} // namespace SysCore::Scripting
