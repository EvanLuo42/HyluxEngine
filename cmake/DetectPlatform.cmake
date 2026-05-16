# =============================================================================
# DetectPlatform.cmake
#
# Detects the active target platform and architecture from the configured
# toolchain (NOT from the host). Console SDKs (PS4/PS5/Xbox/Switch) set their
# own CMAKE_SYSTEM_NAME values via their toolchain file; we map those to a
# stable canonical name the rest of the build can branch on.
#
# Variables set after include():
#
#   HYLUX_PLATFORM            Canonical platform string:
#                             Windows | Linux |
#                             PS4 | PS5 | XboxOne | XboxSeries | Switch |
#                             Unknown
#   HYLUX_PLATFORM_<NAME>     Boolean (TRUE for the active platform only)
#   HYLUX_PLATFORM_CONSOLE    TRUE if PS4/PS5/XboxOne/XboxSeries/Switch
#   HYLUX_PLATFORM_DESKTOP    TRUE if Windows/Linux
#
#   HYLUX_ARCH                Canonical arch string:
#                             X64 | ARM64 | Unknown
#   HYLUX_ARCH_<NAME>         Boolean (TRUE for the active arch only)
#
# Usage:
#   include(DetectPlatform)
#   if(HYLUX_PLATFORM_PS5)
#       target_compile_definitions(MyTarget PRIVATE HYLUX_PS5=1)
#   endif()
# =============================================================================

if (DEFINED HYLUX_PLATFORM AND DEFINED HYLUX_ARCH)
    return()
endif ()

set(_hylux_known_platforms
        Windows Linux
        PS4 PS5 XboxOne XboxSeries Switch
        Unknown
)

set(HYLUX_PLATFORM "Unknown")

if (CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    set(HYLUX_PLATFORM "Windows")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(HYLUX_PLATFORM "Linux")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Orbis")                                     # Sony PS4
    set(HYLUX_PLATFORM "PS4")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Prospero")                                  # Sony PS5
    set(HYLUX_PLATFORM "PS5")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Durango")                                   # MS Xbox One (XDK)
    set(HYLUX_PLATFORM "XboxOne")
elseif (CMAKE_SYSTEM_NAME MATCHES "^(Scarlett|GamingXboxScarlett|GDK_Scarlett)$") # MS Xbox Series (GDK)
    set(HYLUX_PLATFORM "XboxSeries")
elseif (CMAKE_SYSTEM_NAME STREQUAL "NX" OR CMAKE_SYSTEM_NAME STREQUAL "Switch") # Nintendo Switch (NX SDK)
    set(HYLUX_PLATFORM "Switch")
endif ()

foreach (_p IN LISTS _hylux_known_platforms)
    string(TOUPPER "${_p}" _p_upper)
    if (HYLUX_PLATFORM STREQUAL "${_p}")
        set(HYLUX_PLATFORM_${_p_upper} TRUE)
    else ()
        set(HYLUX_PLATFORM_${_p_upper} FALSE)
    endif ()
endforeach ()

if (HYLUX_PLATFORM_PS4 OR HYLUX_PLATFORM_PS5
        OR HYLUX_PLATFORM_XBOXONE OR HYLUX_PLATFORM_XBOXSERIES
        OR HYLUX_PLATFORM_SWITCH)
    set(HYLUX_PLATFORM_CONSOLE TRUE)
else ()
    set(HYLUX_PLATFORM_CONSOLE FALSE)
endif ()

if (HYLUX_PLATFORM_WINDOWS OR HYLUX_PLATFORM_LINUX)
    set(HYLUX_PLATFORM_DESKTOP TRUE)
else ()
    set(HYLUX_PLATFORM_DESKTOP FALSE)
endif ()

set(_hylux_known_archs X64 ARM64 Unknown)

set(HYLUX_ARCH "Unknown")
set(_proc "")

if (DEFINED CMAKE_SYSTEM_PROCESSOR)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _proc)
endif ()

if (MSVC)
    if (DEFINED MSVC_CXX_ARCHITECTURE_ID)
        string(TOLOWER "${MSVC_CXX_ARCHITECTURE_ID}" _proc)
    elseif (DEFINED MSVC_C_ARCHITECTURE_ID)
        string(TOLOWER "${MSVC_C_ARCHITECTURE_ID}" _proc)
    endif ()
endif ()

if (_proc MATCHES "^(amd64|x86_64|x64)$")
    set(HYLUX_ARCH "X64")
elseif (_proc MATCHES "^(aarch64|arm64|arm64e)$")
    set(HYLUX_ARCH "ARM64")
else ()
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(HYLUX_ARCH "X64")
    endif ()
endif ()

if (HYLUX_PLATFORM_PS4 OR HYLUX_PLATFORM_PS5
        OR HYLUX_PLATFORM_XBOXONE OR HYLUX_PLATFORM_XBOXSERIES)
    set(HYLUX_ARCH "X64")
elseif (HYLUX_PLATFORM_SWITCH)
    set(HYLUX_ARCH "ARM64")
endif ()

foreach (_a IN LISTS _hylux_known_archs)
    if (HYLUX_ARCH STREQUAL "${_a}")
        set(HYLUX_ARCH_${_a} TRUE)
    else ()
        set(HYLUX_ARCH_${_a} FALSE)
    endif ()
endforeach ()

message(STATUS "Hylux: platform = ${HYLUX_PLATFORM}, arch = ${HYLUX_ARCH}")

unset(_hylux_known_platforms)
unset(_hylux_known_archs)
unset(_proc)
