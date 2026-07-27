cmake_minimum_required(VERSION 3.20)
project(CryptoIntegrityModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(CryptoIntegrityModule SHARED
    crypto_integrity.cpp
)

target_include_directories(CryptoIntegrityModule PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(CryptoIntegrityModule PRIVATE SystemCoreLib bcrypt)

set_target_properties(CryptoIntegrityModule PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules"
)
