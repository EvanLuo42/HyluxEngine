/// @file
/// @brief Log category split into a runtime base (name + atomic level) and a templated
///        compile-time-min wrapper that enables if-constexpr elimination at call sites.

#pragma once

#include "Core/Logging/LogLevel.h"

#include <atomic>

namespace Hylux
{

/// @brief Runtime portion of a log category. All sinks/dispatchers see categories through this base.
struct LogCategoryBase
{
    const char*           name;
    std::atomic<LogLevel> runtimeMin;
};

/// @brief Compile-time-typed log category. CompileMinV is encoded in the type so the HYLUX_LOG
///        macro can branch on it through `if constexpr` and strip lower-severity call sites
///        entirely in shipping builds.
template <LogLevel CompileMinV>
struct LogCategory : LogCategoryBase
{
    static constexpr LogLevel kCompileMin = CompileMinV;
};

} // namespace Hylux

/// @brief Declares an extern LogCategory in a header. Use HYLUX_DEFINE_LOG_CATEGORY in one .cpp.
#define HYLUX_DECLARE_LOG_CATEGORY_EXTERN(CategoryName, RuntimeDefault, CompileMin)                \
    extern ::Hylux::LogCategory<::Hylux::LogLevel::CompileMin> CategoryName

/// @brief Defines storage for a LogCategory previously declared with HYLUX_DECLARE_LOG_CATEGORY_EXTERN.
#define HYLUX_DEFINE_LOG_CATEGORY(CategoryName, RuntimeDefault, CompileMin)                        \
    ::Hylux::LogCategory<::Hylux::LogLevel::CompileMin> CategoryName{                              \
        ::Hylux::LogCategoryBase{ #CategoryName, { ::Hylux::LogLevel::RuntimeDefault } }           \
    }
