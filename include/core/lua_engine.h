#ifndef SYSCORE_LUA_ENGINE_H
#define SYSCORE_LUA_ENGINE_H

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <sstream>

namespace SysCore::Scripting {

    struct ScriptExecutionResult {
        bool success{false};
        std::string outputLog;
        std::string errorMessage;
        double executionTimeMs{0.0};
    };

    class LuaScriptEngine {
    public:
        LuaScriptEngine() = default;
        ~LuaScriptEngine() = default;

        // Execute a Lua-style diagnostic script string
        [[nodiscard]] static ScriptExecutionResult ExecuteScript(std::string_view scriptSource) noexcept;
        
        // Get preset template scripts
        [[nodiscard]] static std::vector<std::pair<std::string, std::string>> GetPresetTemplates() noexcept;
    };

} // namespace SysCore::Scripting

#endif // SYSCORE_LUA_ENGINE_H
