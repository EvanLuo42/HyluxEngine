/// @file
/// @brief Assertion-handler implementation. Default writes a one-line message to stderr
///        and aborts; an atomic global pointer lets tests / tools swap in a custom
///        handler at runtime.

#include "Core/Utils/Assert.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>

namespace Hylux::Diagnostics
{

namespace
{

std::atomic<AssertionHandler> g_handler{&DefaultAssertionHandler};

} // namespace

[[noreturn]] void DefaultAssertionHandler(const char* file, int line, const char* expr, const char* message)
{
    std::fprintf(stderr, "[Hylux] Assertion failed: %s\n  at %s:%d\n", expr ? expr : "<null>",
                 file ? file : "<unknown>", line);
    if (message != nullptr && message[0] != '\0')
    {
        std::fprintf(stderr, "  message: %s\n", message);
    }
    std::fflush(stderr);
    std::abort();
}

AssertionHandler GetAssertionHandler() noexcept
{
    return g_handler.load(std::memory_order_acquire);
}

AssertionHandler SetAssertionHandler(AssertionHandler newHandler) noexcept
{
    AssertionHandler resolved = newHandler != nullptr ? newHandler : &DefaultAssertionHandler;
    return g_handler.exchange(resolved, std::memory_order_acq_rel);
}

void DispatchAssertionFailure(const char* file, int line, const char* expr, const char* message)
{
    AssertionHandler handler = g_handler.load(std::memory_order_acquire);
    if (handler != nullptr)
    {
        handler(file, line, expr, message);
    }
}

} // namespace Hylux::Diagnostics
