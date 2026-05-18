/// @file
/// @brief IAssetStreamer — coarse asset-level residency planner that sits above
///        AssetSubsystem. The streamer is the layer that translates *world state*
///        (camera position, visible cells, LOD distance, gameplay phase) into
///        *load / keep-resident / release* decisions. It does so by calling
///        AssetSubsystem::Load (to warm the LRU) and Release / SetBudget (to drive
///        eviction). AssetSubsystem itself is intentionally passive — only knows
///        "the user asked for X, hand it back" — so any prefetch, priority ordering,
///        distance-based eviction, or LOD selection lives here.
///
///        This header only declares the interface; the engine ships without a default
///        implementation. Games / editors that want streaming implement
///        IAssetStreamer against their own world model and register the instance with
///        whoever needs it (gameplay code, world subsystem, etc.). Sub-asset page
///        streaming (VT pages, virtual-mesh clusters) is a separate concern handled
///        inside the VirtualTextureAsset / VirtualMeshAsset backends and is not part
///        of this interface.

#pragma once

#include "Asset/AssetBase.h"
#include "Core/Async/CancellationToken.h"
#include "Core/Async/Future.h"
#include "Core/Guid.h"
#include "Core/Memory/Ref.h"

#include <cstdint>
#include <span>

namespace Hylux::Asset
{

/// @brief Ordering hint for the streamer's internal scheduler. Higher values are
///        loaded earlier and evicted later. The Critical band is reserved for
///        gameplay-blocking loads (player character, current level geometry); the
///        streamer is allowed to assume these never get budget-canceled.
enum class StreamPriority : std::uint8_t
{
    Low      = 0,
    Normal   = 1,
    High     = 2,
    Critical = 3,
};

/// @brief One unit of work for the streamer. @p distanceMeters is a tie-breaker
///        within a priority band (closer = earlier). @p token lets the requester
///        revoke the request — the streamer drops it from its queue and propagates
///        cancel into AssetSubsystem::Load via the uploader's token plumbing.
struct StreamRequest
{
    Guid              guid;
    StreamPriority    priority{StreamPriority::Normal};
    float             distanceMeters{0.0f};
    CancellationToken token{};
};

/// @brief Asset-level residency planner. Implementations marshal Request / Release
///        through their own scheduler and call into AssetSubsystem for the actual
///        Load / cache eviction. Thread-safe expectations are
///        implementation-defined; callers should consult the concrete streamer's
///        docs. The methods are virtual rather than templated so the interface can
///        cross DLL / module boundaries.
class IAssetStreamer
{
public:
    virtual ~IAssetStreamer() = default;

    /// @brief Asks the streamer to make @p request.guid resident. Returns a Future
    ///        that resolves to the loaded asset once it is fully constructed (cache
    ///        hit ⇒ already-ready future). Repeated calls for the same guid before
    ///        resolution share the same in-flight Future via AssetSubsystem's
    ///        in-flight dedup.
    [[nodiscard]] virtual Future<Ref<AssetBase>> Request(StreamRequest request) = 0;

    /// @brief Hints that @p guids are likely to be needed soon and the streamer
    ///        should prefetch them when budget allows. The streamer is free to
    ///        ignore the hint under pressure; hinted requests are not tracked with
    ///        Futures.
    virtual void Prefetch(std::span<const StreamRequest> hints) = 0;

    /// @brief Hints that the caller no longer needs @p guid resident. The streamer
    ///        may evict eagerly or defer until budget pressure — the asset itself
    ///        remains alive as long as any external Ref exists (AssetSubsystem's
    ///        WeakRef-aware LRU handles that), so Release is purely advisory.
    virtual void Release(Guid guid) = 0;

    /// @brief Soft byte budget the streamer may keep in flight at once across all
    ///        non-Critical requests. Above the budget, the streamer defers
    ///        lower-priority requests until in-flight bytes drain. Critical
    ///        requests bypass the budget.
    virtual void SetBudget(std::uint64_t bytes) = 0;

    /// @brief Current in-flight + resident bytes the streamer is accounting against
    ///        the budget. Snapshot only — racy by definition.
    [[nodiscard]] virtual std::uint64_t GetBytesInUse() const noexcept = 0;
};

} // namespace Hylux::Asset
