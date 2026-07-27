cmake_minimum_required(VERSION 3.20)
project(ModularSystemFramework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Static C++ Runtime (/MT for Release, /MTd for Debug)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Enforce MSVC C++20 and UTF-8 flags / GCC & Clang flags
if (MSVC)
    add_compile_options(/std:c++20 /W4 /utf-8)
else()
    add_compile_options(-std=c++20 -Wall -Wextra)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core framework sources
set(CORE_SOURCES
    src/core/logger.cpp
    src/core/resource_manager.cpp
    src/core/module_manager.cpp
    src/core/thread_manager.cpp
    src/core/ipc_manager.cpp
    src/core/config_manager.cpp
    src/core/profiler.cpp
    src/core/network_manager.cpp
    src/core/hot_patch_manager.cpp
    src/core/hardware_info.cpp
    src/core/crypto_hash.cpp
    src/core/http_server.cpp
    src/core/async_file.cpp
    src/core/service_manager.cpp
    src/core/benchmark.cpp
    src/core/registry_manager.cpp
    src/core/pipe_manager.cpp
    src/core/driver_info.cpp
    src/core/token_manager.cpp
    src/core/file_watcher.cpp
    src/core/lua_engine.cpp
    src/core/network_monitor.cpp
    src/core/process_tree.cpp
    src/core/core.cpp
)

# Core static library
add_library(SystemCoreLib STATIC ${CORE_SOURCES})
target_include_directories(SystemCoreLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (WIN32)
    target_link_libraries(SystemCoreLib PUBLIC ws2_32 bcrypt iphlpapi)
else()
    target_link_libraries(SystemCoreLib PUBLIC pthread dl)
endif()

# Main Executable
add_executable(SystemCoreApp src/main.cpp)
target_link_libraries(SystemCoreApp PRIVATE SystemCoreLib)

# Unit Test Executable
add_executable(SysCoreUnitTestApp tests/test_main.cpp)
target_link_libraries(SysCoreUnitTestApp PRIVATE SystemCoreLib)

# Modern Desktop GUI Dashboard Executable (Windows DX11)
if (WIN32)
    set(IMGUI_SOURCES
        src/gui/imgui/imgui.cpp
        src/gui/imgui/imgui_draw.cpp
        src/gui/imgui/imgui_widgets.cpp
        src/gui/imgui/imgui_tables.cpp
        src/gui/imgui/imgui_demo.cpp
        src/gui/imgui/imgui_impl_win32.cpp
        src/gui/imgui/imgui_impl_dx11.cpp
    )

    add_executable(SysCoreGuiApp WIN32 src/gui/gui_main.cpp ${IMGUI_SOURCES})
    target_include_directories(SysCoreGuiApp PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include/gui/imgui 
        ${CMAKE_CURRENT_SOURCE_DIR}/include 
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/memory_diagnostic
    )
    target_link_libraries(SysCoreGuiApp PRIVATE SystemCoreLib d3d11.lib dxgi.lib dwmapi.lib ws2_32.lib bcrypt.lib)
endif()

# Add dynamic modules
add_subdirectory(modules/sample_module)
add_subdirectory(modules/memory_diagnostic)
add_subdirectory(modules/system_metrics)
add_subdirectory(modules/process_watcher)
add_subdirectory(modules/network_diagnostic)
add_subdirectory(modules/crypto_integrity)
add_subdirectory(modules/disk_diagnostic)
add_subdirectory(modules/service_diagnostic)
add_subdirectory(modules/registry_diagnostic)
add_subdirectory(modules/pipe_diagnostic)
add_subdirectory(modules/driver_diagnostic)
add_subdirectory(modules/security_diagnostic)
add_subdirectory(modules/file_watcher)
