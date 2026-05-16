/// @file
/// @brief Compiler and calling-convention macros used by the Hylux math library.

#pragma once

#if defined(_MSC_VER)
#define HYLUX_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define HYLUX_FORCEINLINE inline __attribute__((always_inline))
#else
#define HYLUX_FORCEINLINE inline
#endif

#if defined(_MSC_VER) && defined(HYLUX_ARCH_X64)
#define HYLUX_VECTORCALL __vectorcall
#else
#define HYLUX_VECTORCALL
#endif

#define HYLUX_ALIGN(N) alignas(N)
