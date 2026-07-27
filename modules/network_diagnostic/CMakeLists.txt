cmake_minimum_required(VERSION 3.20)
project(NetworkDiagnosticModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(NetworkDiagnosticModule SHARED
    network_diagnostic.cpp
)

target_include_directories(NetworkDiagnosticModule PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(NetworkDiagnosticModule PRIVATE SystemCoreLib ws2_32 iphlpapi)

set_target_properties(NetworkDiagnosticModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules"
)
