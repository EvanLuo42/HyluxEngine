/// @file
/// @brief RAII scope-exit helpers. ScopeGuard<F> calls a callable in its destructor unless
///        Dismiss() was called first. MakeScopeGuard deduces the callable type; the
///        HYLUX_DEFER macro provides Go-style `defer { ... };` syntax that captures the
///        enclosing scope by reference.

#pragma once

#include <type_traits>
#include <utility>

namespace Hylux
{

/// @brief RAII guard that invokes a callable at scope exit. Move-constructible (the moved
///        guard is dismissed), not copyable. The callable must be invocable with no
///        arguments and should be noexcept — exceptions propagating out of the destructor
///        terminate the program.
template<typename CallableT>
class ScopeGuard
{
public:
    explicit ScopeGuard(CallableT callable) noexcept(std::is_nothrow_move_constructible_v<CallableT>)
        : callable_(std::move(callable))
    {}

    ScopeGuard(ScopeGuard&& other) noexcept(std::is_nothrow_move_constructible_v<CallableT>)
        : callable_(std::move(other.callable_)), active_(other.active_)
    {
        other.active_ = false;
    }

    ScopeGuard(const ScopeGuard&)            = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&)      = delete;

    ~ScopeGuard()
    {
        if (active_)
        {
            callable_();
        }
    }

    /// @brief Cancels the guard so the callable will not run at scope exit.
    void Dismiss() noexcept { active_ = false; }

    [[nodiscard]] bool IsActive() const noexcept { return active_; }

private:
    CallableT callable_;
    bool      active_{true};
};

/// @brief Factory that deduces the callable type so callers do not have to spell it out.
template<typename CallableT>
[[nodiscard]] ScopeGuard<std::decay_t<CallableT>> MakeScopeGuard(CallableT&& callable)
{
    return ScopeGuard<std::decay_t<CallableT>>(std::forward<CallableT>(callable));
}

namespace Detail
{

/// @brief Implementation detail behind HYLUX_DEFER — `operator->*` lets the macro chain
///        a lambda literal without parentheses or a trailing semicolon weirdness.
struct ScopeGuardOnExit
{
    template<typename CallableT>
    [[nodiscard]] ScopeGuard<std::decay_t<CallableT>> operator->*(CallableT&& callable) const
    {
        return ScopeGuard<std::decay_t<CallableT>>(std::forward<CallableT>(callable));
    }
};

} // namespace Detail

} // namespace Hylux

#define HYLUX_DEFER_CONCAT_INNER(a, b) a##b
#define HYLUX_DEFER_CONCAT(a, b)       HYLUX_DEFER_CONCAT_INNER(a, b)

/// @brief Go-style scope-exit defer. Usage: `HYLUX_DEFER { fclose(f); };`. Captures the
///        enclosing scope by reference; rebinds at the line it appears on, so multiple
///        defers in the same scope are fine. Each defer runs in reverse declaration order.
#define HYLUX_DEFER \
    [[maybe_unused]] auto HYLUX_DEFER_CONCAT(hylux_defer_, __LINE__) = \
        ::Hylux::Detail::ScopeGuardOnExit{} ->* [&]() noexcept
