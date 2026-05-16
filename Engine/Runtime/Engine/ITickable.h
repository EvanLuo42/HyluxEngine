/// @file
/// @brief Per-frame work-unit interface registered with the Engine.

#pragma once

namespace Hylux
{

/// @brief Per-frame work unit registered with the Engine. Lower TickOrder runs earlier.
class ITickable
{
public:
    virtual ~ITickable() = default;

    /// @brief Invoked once per Engine::Tick after pending registrations have been applied.
    virtual void Tick(float deltaSeconds) = 0;

    /// @brief Sort key for tick ordering. Ties are resolved by registration order (stable sort).
    [[nodiscard]] virtual int TickOrder() const { return 0; }
};

} // namespace Hylux
