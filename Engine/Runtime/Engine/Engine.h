/// @file
/// @brief Engine runtime orchestrator: owns subsystems, schedules tickables, drives the per-frame tick.

#pragma once

#include "Core/Reflection/TypeId.h"
#include "Engine/ISubsystem.h"
#include "Engine/ITickable.h"

#include <cassert>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Hylux
{

/// @brief Runtime orchestrator. Owns subsystem lifetimes, resolves dependency ordering
///        via Kahn topological sort, and drives a per-frame tickable list.
///        Not a singleton: construct one per host (editor world, standalone launcher, test).
///        All public methods must be called from the same thread (the main loop).
class Engine
{
public:
    Engine() = default;
    ~Engine() = default;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    /// @brief Resolves subsystem init order from dependencies, then calls Initialize on each.
    void Initialize();

    /// @brief Calls Shutdown on every subsystem in reverse init order and destroys instances.
    void Shutdown();

    /// @brief Drains pending tickable registrations, re-sorts if dirty, then ticks all.
    void Tick(float deltaSeconds);

    /// @brief Constructs T in-place and registers it as a subsystem. Must run before Initialize.
    template<typename T, typename... Args> T* RegisterSubsystem(Args&&... args);

    /// @brief Returns the subsystem of type T, or nullptr if not registered. O(1).
    template<typename T> [[nodiscard]] T* GetSubsystem() const noexcept;

    /// @brief Queues a tickable for inclusion on the next Tick. Non-owning.
    void RegisterTickable(ITickable* tickable);

    /// @brief Queues a tickable for removal on the next Tick. Silent no-op if unknown.
    void UnregisterTickable(ITickable* tickable);

    [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }

private:
    void BuildInitOrder();
    void ApplyPendingTickables();

    struct SubsystemSlot
    {
        std::uint64_t id;
        std::unique_ptr<ISubsystem> instance;
    };

    std::vector<SubsystemSlot> subsystems_;
    std::vector<ISubsystem*> initOrder_;
    std::unordered_map<std::uint64_t, ISubsystem*> subsystemById_;

    std::vector<ITickable*> tickables_;
    std::vector<ITickable*> pendingAdd_;
    std::vector<ITickable*> pendingRemove_;
    bool tickablesDirty_{false};
    bool initialized_{false};
};

template<typename T, typename... Args> T* Engine::RegisterSubsystem(Args&&... args)
{
    static_assert(std::is_base_of_v<ISubsystem, T>, "T must derive from ISubsystem");
    assert(!initialized_ && "RegisterSubsystem must be called before Initialize()");
    const std::uint64_t id = ToU64(TypeIdOf<T>());
    assert(subsystemById_.find(id) == subsystemById_.end() && "Duplicate subsystem registration");
    auto owned = std::make_unique<T>(std::forward<Args>(args)...);
    T* raw = owned.get();
    subsystemById_.emplace(id, static_cast<ISubsystem*>(raw));
    subsystems_.push_back(SubsystemSlot{id, std::move(owned)});
    return raw;
}

template<typename T> T* Engine::GetSubsystem() const noexcept
{
    const std::uint64_t id = ToU64(TypeIdOf<T>());
    auto it = subsystemById_.find(id);
    return it == subsystemById_.end() ? nullptr : static_cast<T*>(it->second);
}

} // namespace Hylux
