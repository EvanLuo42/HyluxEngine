/// @file
/// @brief Tiered assertion macros and pluggable assertion handler. HYLUX_ASSERT is the
///        debug-only guard (compiled out in any build that defines NDEBUG); HYLUX_CHECK
///        keeps firing in Dev configurations (Debug + RelWithDebInfo); HYLUX_VERIFY runs
///        in every config and the expression is always evaluated regardless of build
///        level — wrap fallible calls whose side effects must happen unconditionally.

#pragma once

namespace Hylux::Diagnostics
{

#if defined(_DEBUG) || !defined(NDEBUG)
inline constexpr bool kBuildIsDebug = true;
#else
inline constexpr bool kBuildIsDebug = false;
#endif

#if defined(HYLUX_LOG_LEVEL_MIN) && HYLUX_LOG_LEVEL_MIN <= 1
inline constexpr bool kBuildIsDev = true;
#else
inline constexpr bool kBuildIsDev = kBuildIsDebug;
#endif

/// @brief Signature of a process-wide assertion handler. Receives the source location
///        plus a stringified expression and an optional message literal. The default
///        handler logs to stderr and aborts; test code can swap in a handler that throws
///        or sets a flag to inspect failures non-fatally.
using AssertionHandler = void (*)(const char* file, int line, const char* expr, const char* message);

/// @brief Default handler. Writes a one-line diagnostic to stderr and calls std::abort.
[[noreturn]] void DefaultAssertionHandler(const char* file, int line, const char* expr, const char* message);

/// @brief Returns the currently-installed handler.
[[nodiscard]] AssertionHandler GetAssertionHandler() noexcept;

/// @brief Replaces the handler. Passing nullptr restores DefaultAssertionHandler. Returns
///        the previous handler so callers can scope-restore it.
AssertionHandler SetAssertionHandler(AssertionHandler newHandler) noexcept;

/// @brief Dispatches a failure to the current handler. Not marked [[noreturn]] because a
///        custom handler is allowed to return (tests). When the default handler is active
///        the program terminates inside the handler.
void DispatchAssertionFailure(const char* file, int line, const char* expr, const char* message);

} // namespace Hylux::Diagnostics

#define HYLUX_ASSERT_DISPATCH(expr, message) \
    ::Hylux::Diagnostics::DispatchAssertionFailure(__FILE__, __LINE__, expr, message)

/// @brief Debug-only invariant. Compiled out in any build that defines NDEBUG.
#define HYLUX_ASSERT(expr)                                          \
    do {                                                            \
        if constexpr (::Hylux::Diagnostics::kBuildIsDebug)          \
        {                                                           \
            if (!(expr)) { HYLUX_ASSERT_DISPATCH(#expr, ""); }      \
        }                                                           \
    } while (0)

/// @brief Same as HYLUX_ASSERT but accepts a literal C-string message.
#define HYLUX_ASSERT_MSG(expr, message)                             \
    do {                                                            \
        if constexpr (::Hylux::Diagnostics::kBuildIsDebug)          \
        {                                                           \
            if (!(expr)) { HYLUX_ASSERT_DISPATCH(#expr, message); } \
        }                                                           \
    } while (0)

/// @brief Dev-tier invariant: enabled in Debug and RelWithDebInfo, compiled out in
///        Release / MinSizeRel. Use for checks that should survive into developer
///        builds but not into shipping binaries.
#define HYLUX_CHECK(expr)                                           \
    do {                                                            \
        if constexpr (::Hylux::Diagnostics::kBuildIsDev)            \
        {                                                           \
            if (!(expr)) { HYLUX_ASSERT_DISPATCH(#expr, ""); }      \
        }                                                           \
    } while (0)

/// @brief Same as HYLUX_CHECK with a literal C-string message.
#define HYLUX_CHECK_MSG(expr, message)                              \
    do {                                                            \
        if constexpr (::Hylux::Diagnostics::kBuildIsDev)            \
        {                                                           \
            if (!(expr)) { HYLUX_ASSERT_DISPATCH(#expr, message); } \
        }                                                           \
    } while (0)

/// @brief Always evaluates `expr` (so wrapping fallible calls with side effects is
///        safe). In Dev builds a falsy result fires the handler; in Release builds
///        the result is discarded and execution continues.
#define HYLUX_VERIFY(expr)                                          \
    do {                                                            \
        const bool _hylux_verify_ok = static_cast<bool>(expr);      \
        if constexpr (::Hylux::Diagnostics::kBuildIsDev)            \
        {                                                           \
            if (!_hylux_verify_ok) { HYLUX_ASSERT_DISPATCH(#expr, ""); } \
        }                                                           \
        else                                                        \
        {                                                           \
            (void)_hylux_verify_ok;                                 \
        }                                                           \
    } while (0)
