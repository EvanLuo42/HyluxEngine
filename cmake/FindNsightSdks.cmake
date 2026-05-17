# =============================================================================
# FindNsightSdks.cmake
#
# Locates the NVIDIA Nsight Graphics SDK (NGFX) and NVIDIA Nsight Aftermath
# SDK shipped under a single Nsight Graphics installation. Both default to the
# root at HYLUX_NSIGHT_SDK_ROOT (set in the top-level CMakeLists from the env
# var NSIGHT_GRAPHICS_SDK_ROOT or the well-known Windows install path).
#
# Targets produced when the corresponding subdirectory exists:
#   Nvidia::NsightGraphics  INTERFACE  -- NGFX header-only include path
#   Nvidia::Aftermath       SHARED IMPORTED -- GFSDK_Aftermath_Lib.x64.{lib,dll}
#                                              + include path
#
# Both targets are skipped when files are missing; callers must guard
# `target_link_libraries(... PRIVATE Nvidia::Xxx)` with `if (TARGET ...)`.
# =============================================================================

if (TARGET Nvidia::NsightGraphics AND TARGET Nvidia::Aftermath)
    return()
endif ()

if (NOT HYLUX_NSIGHT_SDK_ROOT OR NOT EXISTS "${HYLUX_NSIGHT_SDK_ROOT}")
    message(STATUS "FindNsightSdks: HYLUX_NSIGHT_SDK_ROOT not set or missing; "
                   "skipping Nsight integrations.")
    return()
endif ()

# --- NGFX --------------------------------------------------------------------
if (NOT TARGET Nvidia::NsightGraphics)
    file(GLOB _ngfx_versions LIST_DIRECTORIES true
         "${HYLUX_NSIGHT_SDK_ROOT}/NsightGraphicsSDK/*")
    list(FILTER _ngfx_versions INCLUDE REGEX "[0-9]")
    list(SORT _ngfx_versions ORDER DESCENDING)
    if (_ngfx_versions)
        list(GET _ngfx_versions 0 _ngfx_root)
        set(_ngfx_inc "${_ngfx_root}/include")
        if (EXISTS "${_ngfx_inc}/NGFX_Vulkan.h")
            add_library(Nvidia_NsightGraphics INTERFACE)
            add_library(Nvidia::NsightGraphics ALIAS Nvidia_NsightGraphics)
            target_include_directories(Nvidia_NsightGraphics INTERFACE "${_ngfx_inc}")
            message(STATUS "FindNsightSdks: NGFX  = ${_ngfx_root}")
        else ()
            message(STATUS "FindNsightSdks: NGFX include dir missing NGFX_Vulkan.h "
                           "at ${_ngfx_inc}; skipping.")
        endif ()
    else ()
        message(STATUS "FindNsightSdks: no NsightGraphicsSDK/<ver>/ under "
                       "${HYLUX_NSIGHT_SDK_ROOT}; skipping NGFX.")
    endif ()
endif ()

# --- Aftermath ---------------------------------------------------------------
if (NOT TARGET Nvidia::Aftermath)
    file(GLOB _aftermath_versions LIST_DIRECTORIES true
         "${HYLUX_NSIGHT_SDK_ROOT}/NsightAftermathSDK/*")
    list(FILTER _aftermath_versions INCLUDE REGEX "[0-9]")
    list(SORT _aftermath_versions ORDER DESCENDING)
    if (_aftermath_versions)
        list(GET _aftermath_versions 0 _aftermath_root)
        set(_aftermath_inc "${_aftermath_root}/include")
        set(_aftermath_lib "${_aftermath_root}/lib/x64/GFSDK_Aftermath_Lib.x64.lib")
        set(_aftermath_dll "${_aftermath_root}/lib/x64/GFSDK_Aftermath_Lib.x64.dll")
        if (EXISTS "${_aftermath_inc}/GFSDK_Aftermath.h"
            AND EXISTS "${_aftermath_lib}"
            AND EXISTS "${_aftermath_dll}")
            add_library(Nvidia_Aftermath SHARED IMPORTED)
            add_library(Nvidia::Aftermath ALIAS Nvidia_Aftermath)
            set_target_properties(Nvidia_Aftermath PROPERTIES
                IMPORTED_LOCATION             "${_aftermath_dll}"
                IMPORTED_IMPLIB               "${_aftermath_lib}"
                INTERFACE_INCLUDE_DIRECTORIES "${_aftermath_inc}")
            message(STATUS "FindNsightSdks: Aftermath = ${_aftermath_root}")
        else ()
            message(STATUS "FindNsightSdks: Aftermath files incomplete under "
                           "${_aftermath_root}; skipping.")
        endif ()
    else ()
        message(STATUS "FindNsightSdks: no NsightAftermathSDK/<ver>/ under "
                       "${HYLUX_NSIGHT_SDK_ROOT}; skipping Aftermath.")
    endif ()
endif ()
