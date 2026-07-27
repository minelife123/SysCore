cmake_minimum_required(VERSION 3.20)
project(FileWatcherModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(FileWatcherModule SHARED
    file_watcher_module.cpp
)

target_include_directories(FileWatcherModule PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(FileWatcherModule PRIVATE SystemCoreLib)

set_target_properties(FileWatcherModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules"
)
