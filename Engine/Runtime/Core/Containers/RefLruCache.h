/// @file
/// @brief Refcount-aware LRU. Caches Ref<ValueT> by KeyT with a per-entry footprint
///        against a soft byte budget; evicting an entry drops only the cache's strong
///        reference, so external Ref<> consumers still keep the value alive. A WeakRef
///        per slot lets the cache resurrect a still-live value before the caller has to
///        re-load it from its source.

#pragma once

#include "Core/Memory/Ref.h"
#include "Core/Memory/RefCounted.h"
#include "Core/Memory/WeakRef.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <unordered_map>
#include <utility>

namespace Hylux
{

/// @brief Refcount-aware LRU.
///
/// Invariants:
///   - bytesInUse_ == sum of slot.footprint for every slot whose strong ref is non-null.
///   - slot.order is a valid iterator into order_ iff slot.strong is non-null.
///   - order_.front() is the least-recently-used strong entry; back() is most-recent.
///   - A slot may live in slots_ with a null strong but a still-lockable weak; that slot
///     is invisible to the budget but visible to Get() for resurrection. The slot is
///     erased from slots_ the first time Get() observes the weak as expired.
///
/// Single-threaded by design — callers serialise access (the AssetSubsystem holds the
/// cache and is already game-thread-only).
template<typename KeyT, typename ValueT, typename KeyHash = std::hash<KeyT>>
class RefLruCache
{
    static_assert(std::is_base_of_v<RefCounted, ValueT>,
                  "RefLruCache<K, V>: V must publicly derive from Hylux::RefCounted");

public:
    explicit RefLruCache(std::uint64_t budgetBytes) noexcept : budget_(budgetBytes) {}

    RefLruCache(const RefLruCache&)            = delete;
    RefLruCache& operator=(const RefLruCache&) = delete;

    /// @brief Looks up by key. Returns a Ref to a still-live value; null if the slot is
    ///        absent or both the strong and weak refs are gone. A strong hit promotes the
    ///        entry to MRU. A weak-only hit (cache evicted but external Ref still alive)
    ///        returns the value without re-claiming a strong reference — the cache stays
    ///        evicted, so reads do not churn the budget. Re-caching happens on Insert.
    [[nodiscard]] Ref<ValueT> Get(const KeyT& key)
    {
        auto it = slots_.find(key);
        if (it == slots_.end())
        {
            return Ref<ValueT>{};
        }
        Slot& slot = it->second;
        if (slot.strong)
        {
            order_.splice(order_.end(), order_, slot.order);
            return slot.strong;
        }
        Ref<ValueT> live = slot.weak.Lock();
        if (!live)
        {
            slots_.erase(it);
            return Ref<ValueT>{};
        }
        return live;
    }

    /// @brief Inserts (or replaces). Evicts oldest entries first so the new entry fits.
    ///        The just-inserted entry is never evicted in the same call, even if its
    ///        footprint alone exceeds the budget — over-budget entries simply make the
    ///        budget temporarily exceeded until they fall out via Get-misses.
    void Insert(const KeyT& key, Ref<ValueT> value, std::uint64_t footprint)
    {
        if (!value)
        {
            return;
        }

        auto [it, inserted] = slots_.try_emplace(key);
        Slot& slot          = it->second;
        if (!inserted && slot.strong)
        {
            bytesInUse_ -= slot.footprint;
            order_.erase(slot.order);
            slot.strong.Reset();
        }

        EvictUntilFits(footprint);

        slot.weak      = WeakRef<ValueT>(value);
        slot.strong    = std::move(value);
        slot.footprint = footprint;
        slot.order     = order_.insert(order_.end(), key);
        bytesInUse_ += footprint;
    }

    /// @brief Drops every strong reference held by the cache. WeakRefs are preserved so
    ///        external Ref<> consumers continue to keep their values alive, and a later
    ///        Get() can still resurrect a still-live entry.
    void Clear()
    {
        for (auto& [_, slot] : slots_)
        {
            slot.strong.Reset();
        }
        order_.clear();
        bytesInUse_ = 0;
    }

    [[nodiscard]] std::uint64_t BytesInUse() const noexcept { return bytesInUse_; }
    [[nodiscard]] std::uint64_t Budget() const noexcept { return budget_; }
    [[nodiscard]] std::size_t   Size() const noexcept { return slots_.size(); }

private:
    struct Slot
    {
        Ref<ValueT>                        strong;
        WeakRef<ValueT>                    weak;
        std::uint64_t                      footprint{0};
        typename std::list<KeyT>::iterator order;
    };

    void EvictUntilFits(std::uint64_t reserved)
    {
        while (!order_.empty() && bytesInUse_ + reserved > budget_)
        {
            const KeyT frontKey = order_.front();
            auto       it       = slots_.find(frontKey);
            order_.pop_front();
            if (it == slots_.end())
            {
                continue;
            }
            Slot& slot = it->second;
            bytesInUse_ -= slot.footprint;
            slot.strong.Reset();
        }
    }

    std::uint64_t                            budget_;
    std::uint64_t                            bytesInUse_{0};
    std::list<KeyT>                          order_;
    std::unordered_map<KeyT, Slot, KeyHash>  slots_;
};

} // namespace Hylux
