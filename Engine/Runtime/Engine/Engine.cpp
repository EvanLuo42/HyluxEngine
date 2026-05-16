#include "Engine/Engine.h"

#include <algorithm>
#include <queue>
#include <unordered_set>

namespace Hylux
{

void Engine::BuildInitOrder()
{
    const std::size_t count = subsystems_.size();
    initOrder_.clear();
    initOrder_.reserve(count);

    std::unordered_map<std::uint64_t, std::size_t> indexById;
    indexById.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        indexById.emplace(subsystems_[i].id, i);
    }

    std::vector<std::vector<std::size_t>> dependents(count);
    std::vector<std::size_t> inDegree(count, 0);

    for (std::size_t i = 0; i < count; ++i)
    {
        for (const std::vector<TypeId> deps = subsystems_[i].instance->GetDependencies(); const TypeId dep : deps)
        {
            const std::uint64_t depId = ToU64(dep);
            auto it = indexById.find(depId);
            assert(it != indexById.end() && "Subsystem declared a dependency that was never registered");
            dependents[it->second].push_back(i);
            ++inDegree[i];
        }
    }

    std::queue<std::size_t> ready;
    for (std::size_t i = 0; i < count; ++i)
    {
        if (inDegree[i] == 0)
        {
            ready.push(i);
        }
    }

    while (!ready.empty())
    {
        const std::size_t i = ready.front();
        ready.pop();
        initOrder_.push_back(subsystems_[i].instance.get());
        for (std::size_t j : dependents[i])
        {
            if (--inDegree[j] == 0)
            {
                ready.push(j);
            }
        }
    }

    assert(initOrder_.size() == count && "Subsystem dependency cycle detected");
}

void Engine::Initialize()
{
    assert(!initialized_ && "Engine::Initialize called twice");
    BuildInitOrder();
    for (ISubsystem* s : initOrder_)
    {
        s->Initialize(*this);
    }
    initialized_ = true;
}

void Engine::Shutdown()
{
    if (!initialized_)
    {
        return;
    }
    for (auto it = initOrder_.rbegin(); it != initOrder_.rend(); ++it)
    {
        (*it)->Shutdown();
    }
    tickables_.clear();
    pendingAdd_.clear();
    pendingRemove_.clear();
    tickablesDirty_ = false;
    initOrder_.clear();
    subsystemById_.clear();
    subsystems_.clear();
    initialized_ = false;
}

void Engine::ApplyPendingTickables()
{
    if (!pendingRemove_.empty())
    {
        std::unordered_set toRemove(pendingRemove_.begin(), pendingRemove_.end());
        pendingRemove_.clear();
        tickables_.erase(
            std::ranges::remove_if(tickables_, [&toRemove](ITickable* t) { return toRemove.count(t) > 0; }).begin(),
            tickables_.end());
        for (auto it = pendingAdd_.begin(); it != pendingAdd_.end();)
        {
            if (toRemove.count(*it) > 0)
            {
                it = pendingAdd_.erase(it);
            }
            else
            {
                ++it;
            }
        }
        tickablesDirty_ = true;
    }
    if (!pendingAdd_.empty())
    {
        tickables_.insert(tickables_.end(), pendingAdd_.begin(), pendingAdd_.end());
        pendingAdd_.clear();
        tickablesDirty_ = true;
    }
}

void Engine::Tick(float deltaSeconds)
{
    assert(initialized_ && "Engine::Tick called before Initialize");
    ApplyPendingTickables();
    if (tickablesDirty_)
    {
        std::ranges::stable_sort(
            tickables_, [](const ITickable* a, const ITickable* b) { return a->TickOrder() < b->TickOrder(); });
        tickablesDirty_ = false;
    }
    for (ITickable* t : tickables_)
    {
        t->Tick(deltaSeconds);
    }
}

void Engine::RegisterTickable(ITickable* tickable)
{
    if (tickable == nullptr)
    {
        return;
    }
    pendingAdd_.push_back(tickable);
}

void Engine::UnregisterTickable(ITickable* tickable)
{
    if (tickable == nullptr)
    {
        return;
    }
    pendingRemove_.push_back(tickable);
}

} // namespace Hylux
