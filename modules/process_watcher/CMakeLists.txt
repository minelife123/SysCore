cmake_minimum_required(VERSION 3.20)
project(ProcessWatcherModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ProcessWatcherModule SHARED
    process_watcher.cpp
)

target_include_directories(ProcessWatcherModule PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(ProcessWatcherModule PRIVATE SystemCoreLib)

set_target_properties(ProcessWatcherModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules"
)
