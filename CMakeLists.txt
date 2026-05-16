cmake_minimum_required(VERSION 3.26)

project(HyluxEngine
        VERSION 0.1.0
        DESCRIPTION "HyluxEngine"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

if (MSVC)
    add_compile_options(/utf-8 /Zc:__cplusplus /Zc:preprocessor /permissive- /MP /W4 /EHsc)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(HYLUX_BUILD_EDITOR "Build the Hylux Editor" ON)

include(DetectPlatform)

add_subdirectory(Engine)
