cmake_minimum_required(VERSION 3.20)
project(SampleModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(SampleModule SHARED
    sample_module.cpp
)

target_include_directories(SampleModule PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(SampleModule PRIVATE SystemCoreLib)

# Set output directory to bin/modules
set_target_properties(SampleModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules"
)
