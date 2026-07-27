cmake_minimum_required(VERSION 3.20)
project(MemoryDiagnosticModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(MemoryDiagnosticModule SHARED
    memory_diagnostic.cpp
)

target_include_directories(MemoryDiagnosticModule PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(MemoryDiagnosticModule PRIVATE SystemCoreLib)

set_target_properties(MemoryDiagnosticModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules"
)
