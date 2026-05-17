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
option(HYLUX_BUILD_TESTS  "Build the Hylux test targets" ON)

option(HYLUX_RHI_VULKAN "Build the Vulkan RHI backend" ON)

option(HYLUX_CAPTURE_NSIGHT    "Integrate NVIDIA Nsight Graphics SDK"   OFF)
option(HYLUX_CAPTURE_RENDERDOC "Integrate RenderDoc SDK"                OFF)
option(HYLUX_CAPTURE_PIX       "Integrate WinPixEventRuntime / PIX SDK" OFF)
option(HYLUX_CRASH_AFTERMATH   "Integrate NVIDIA Nsight Aftermath"      OFF)
option(HYLUX_CRASH_DRED        "Integrate D3D12 DRED"                   OFF)

option(HYLUX_CAPTURE_IN_RELEASE         "Also compile capture SDKs into Release/MinSizeRel" OFF)
option(HYLUX_CRASH_AFTERMATH_IN_RELEASE "Also compile Aftermath into Release/MinSizeRel"    OFF)

set(HYLUX_NSIGHT_SDK_ROOT "" CACHE PATH
    "Root containing NsightGraphicsSDK/ and NsightAftermathSDK/ (defaults to env NSIGHT_GRAPHICS_SDK_ROOT)")

if (NOT HYLUX_NSIGHT_SDK_ROOT AND DEFINED ENV{NSIGHT_GRAPHICS_SDK_ROOT})
    set(HYLUX_NSIGHT_SDK_ROOT "$ENV{NSIGHT_GRAPHICS_SDK_ROOT}" CACHE PATH "" FORCE)
endif ()
if (NOT HYLUX_NSIGHT_SDK_ROOT AND WIN32)
    file(GLOB _hylux_nsight_candidates LIST_DIRECTORIES true
         "C:/Program Files/NVIDIA Corporation/Nsight Graphics */SDKs")
    if (_hylux_nsight_candidates)
        list(SORT _hylux_nsight_candidates ORDER DESCENDING)
        list(GET _hylux_nsight_candidates 0 HYLUX_NSIGHT_SDK_ROOT)
        set(HYLUX_NSIGHT_SDK_ROOT "${HYLUX_NSIGHT_SDK_ROOT}" CACHE PATH "" FORCE)
    endif ()
endif ()

if (HYLUX_NSIGHT_SDK_ROOT AND EXISTS "${HYLUX_NSIGHT_SDK_ROOT}")
    message(STATUS "Hylux: NSIGHT SDK root = ${HYLUX_NSIGHT_SDK_ROOT}")
    if (EXISTS "${HYLUX_NSIGHT_SDK_ROOT}/NsightGraphicsSDK" AND NOT HYLUX_CAPTURE_NSIGHT)
        message(STATUS "Hylux: auto-enabling HYLUX_CAPTURE_NSIGHT (SDK present)")
        set(HYLUX_CAPTURE_NSIGHT ON CACHE BOOL "" FORCE)
    endif ()
    if (EXISTS "${HYLUX_NSIGHT_SDK_ROOT}/NsightAftermathSDK" AND NOT HYLUX_CRASH_AFTERMATH)
        message(STATUS "Hylux: auto-enabling HYLUX_CRASH_AFTERMATH (SDK present)")
        set(HYLUX_CRASH_AFTERMATH ON CACHE BOOL "" FORCE)
    endif ()
endif ()

function(_hylux_resolve_dev_config out_var master_var release_opt_var)
    set(${out_var} OFF PARENT_SCOPE)
    if (NOT ${${master_var}})
        return()
    endif ()
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(${out_var} ON PARENT_SCOPE)
    elseif (${${release_opt_var}})
        set(${out_var} ON PARENT_SCOPE)
    endif ()
endfunction()

_hylux_resolve_dev_config(HYLUX_CAPTURE_NSIGHT_ACTIVE    HYLUX_CAPTURE_NSIGHT    HYLUX_CAPTURE_IN_RELEASE)
_hylux_resolve_dev_config(HYLUX_CAPTURE_RENDERDOC_ACTIVE HYLUX_CAPTURE_RENDERDOC HYLUX_CAPTURE_IN_RELEASE)
_hylux_resolve_dev_config(HYLUX_CAPTURE_PIX_ACTIVE       HYLUX_CAPTURE_PIX       HYLUX_CAPTURE_IN_RELEASE)
_hylux_resolve_dev_config(HYLUX_CRASH_AFTERMATH_ACTIVE   HYLUX_CRASH_AFTERMATH   HYLUX_CRASH_AFTERMATH_IN_RELEASE)

message(STATUS "Hylux RHI: vulkan=${HYLUX_RHI_VULKAN} "
               "nsight=${HYLUX_CAPTURE_NSIGHT_ACTIVE} "
               "aftermath=${HYLUX_CRASH_AFTERMATH_ACTIVE} "
               "(build_type=${CMAKE_BUILD_TYPE})")

include(DetectPlatform)

if (HYLUX_PLATFORM_WINDOWS)
    add_compile_definitions(HYLUX_WINDOWS=1)
elseif (HYLUX_PLATFORM_LINUX)
    add_compile_definitions(HYLUX_LINUX=1)
elseif (HYLUX_PLATFORM_PS4)
    add_compile_definitions(HYLUX_PS4=1)
elseif (HYLUX_PLATFORM_PS5)
    add_compile_definitions(HYLUX_PS5=1)
elseif (HYLUX_PLATFORM_XBOXONE)
    add_compile_definitions(HYLUX_XBOXONE=1)
elseif (HYLUX_PLATFORM_XBOXSERIES)
    add_compile_definitions(HYLUX_XBOXSERIES=1)
elseif (HYLUX_PLATFORM_SWITCH)
    add_compile_definitions(HYLUX_SWITCH=1)
endif ()

if (HYLUX_PLATFORM_DESKTOP)
    add_compile_definitions(HYLUX_DESKTOP=1)
elseif (HYLUX_PLATFORM_CONSOLE)
    add_compile_definitions(HYLUX_CONSOLE=1)
endif ()

if (HYLUX_ARCH_X64)
    add_compile_definitions(HYLUX_ARCH_X64=1)
elseif (HYLUX_ARCH_ARM64)
    add_compile_definitions(HYLUX_ARCH_ARM64=1)
endif ()

include(MathConfig)

if (HYLUX_BUILD_TESTS)
    enable_testing()
endif ()

add_subdirectory(Engine)
