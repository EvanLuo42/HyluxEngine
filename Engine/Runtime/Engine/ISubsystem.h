/// @file
/// @brief Abstract subsystem interface owned by the Engine.

#pragma once

#include "Core/Reflection/TypeId.h"

#include <vector>

namespace Hylux
{

class Engine;

/// @brief Abstract base for engine subsystems. Lifetime is owned by Engine.
///        Override GetDependencies to declare initialization ordering against other subsystems.
class ISubsystem
{
public:
    virtual ~ISubsystem() = default;

    /// @brief Called once after every declared dependency has finished initializing.
    virtual void Initialize(Engine& engine) = 0;

    /// @brief Called once during Engine::Shutdown in reverse initialization order.
    virtual void Shutdown() = 0;

    /// @brief TypeIds of subsystems that must initialize before this one. Default: none.
    [[nodiscard]] virtual std::vector<TypeId> GetDependencies() const { return {}; }
};

} // namespace Hylux
