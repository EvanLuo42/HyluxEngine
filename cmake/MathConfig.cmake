# =============================================================================
# MathConfig.cmake
#
# Selects the SIMD backend for Hylux::Core::Math.
#
# Cache options:
#
#   HYLUX_MATH_SIMD_BACKEND          AUTO | SSE | NEON | SCALAR | EXT
#                                    AUTO (default) picks SSE on X64, NEON on
#                                    ARM64, SCALAR otherwise.
#
#   HYLUX_MATH_SIMD_EXTENSION_HEADER Header path (relative to a Runtime include
#                                    directory) implementing the L0 SIMD API.
#                                    Required when backend = EXT. The path is
#                                    forwarded to translation units as the
#                                    HYLUX_MATH_SIMD_EXTENSION_HEADER macro and
#                                    consumed inside Core/Math/Simd.h via
#                                    `#include HYLUX_MATH_SIMD_EXTENSION_HEADER`.
#
# Console platforms typically set EXT in their platform CMake toolchain so a
# vendor-tuned SIMD layer can replace the open-source backend without leaking
# NDA code into this tree.
# =============================================================================

if (DEFINED HYLUX_MATH_CONFIG_DONE)
    return()
endif ()
set(HYLUX_MATH_CONFIG_DONE TRUE)

set(HYLUX_MATH_SIMD_BACKEND "AUTO" CACHE STRING
        "SIMD backend for Hylux::Core::Math (AUTO|SSE|NEON|SCALAR|EXT)")
set_property(CACHE HYLUX_MATH_SIMD_BACKEND PROPERTY STRINGS
        AUTO SSE NEON SCALAR EXT)

set(HYLUX_MATH_SIMD_EXTENSION_HEADER "" CACHE STRING
        "Header providing the L0 SIMD API when HYLUX_MATH_SIMD_BACKEND=EXT")

set(_hylux_math_backend "${HYLUX_MATH_SIMD_BACKEND}")
if (_hylux_math_backend STREQUAL "AUTO")
    if (HYLUX_ARCH_X64)
        set(_hylux_math_backend "SSE")
    elseif (HYLUX_ARCH_ARM64)
        set(_hylux_math_backend "NEON")
    else ()
        set(_hylux_math_backend "SCALAR")
    endif ()
endif ()

if (_hylux_math_backend STREQUAL "SSE")
    add_compile_definitions(HYLUX_MATH_SIMD_BACKEND_SSE=1)
elseif (_hylux_math_backend STREQUAL "NEON")
    add_compile_definitions(HYLUX_MATH_SIMD_BACKEND_NEON=1)
elseif (_hylux_math_backend STREQUAL "SCALAR")
    add_compile_definitions(HYLUX_MATH_SIMD_BACKEND_SCALAR=1)
elseif (_hylux_math_backend STREQUAL "EXT")
    if (HYLUX_MATH_SIMD_EXTENSION_HEADER STREQUAL "")
        message(FATAL_ERROR
                "HYLUX_MATH_SIMD_BACKEND=EXT requires HYLUX_MATH_SIMD_EXTENSION_HEADER to be set.")
    endif ()
    add_compile_definitions(
            HYLUX_MATH_SIMD_BACKEND_EXT=1
            "HYLUX_MATH_SIMD_EXTENSION_HEADER=\"${HYLUX_MATH_SIMD_EXTENSION_HEADER}\""
    )
else ()
    message(FATAL_ERROR
            "Unknown HYLUX_MATH_SIMD_BACKEND='${HYLUX_MATH_SIMD_BACKEND}' (expected AUTO|SSE|NEON|SCALAR|EXT)")
endif ()

message(STATUS "Hylux: math SIMD backend = ${_hylux_math_backend}")
unset(_hylux_math_backend)
