/// @file
/// @brief Persistent texture pool that survives across frames. Render paths import
///        history color, HZB, motion vectors, and other "needs last frame's data"
///        targets by stable name; the pool reallocates only when the requested desc
///        changes (e.g. viewport resize) and evicts entries unused for several frames.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIResourceDesc.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Hylux::Renderer
{

/// @brief Per-renderer persistent texture pool. Construct after the device is available;
///        slots survive across BuildGraph invocations until either the desc changes or
///        EndFrame evicts them as idle.
class RenderResources
{
public:
    explicit RenderResources(RHI::IRHIDevice* device) noexcept;
    ~RenderResources();

    RenderResources(const RenderResources&)            = delete;
    RenderResources& operator=(const RenderResources&) = delete;

    /// @brief Returns the texture stored under `name`, creating it on first request and
    ///        reallocating it whenever the cached desc differs structurally from `desc`.
    ///        Stamps the slot with `renderFrameId` so EndFrame can age out unused entries.
    ///        Returns nullptr only on RHI allocation failure.
    RHI::IRHITexture* GetOrCreateTexture(std::string_view name, const RHI::TextureDesc& desc,
                                         std::uint64_t renderFrameId);

    /// @brief Lookup-only access. Does not touch lastUsedFrameId. Returns nullptr if
    ///        no slot exists.
    [[nodiscard]] RHI::IRHITexture* FindTexture(std::string_view name) const noexcept;

    /// @brief Drops every slot. Caller guarantees no in-flight GPU work still references
    ///        the textures (usually called from Shutdown after WaitIdle).
    void Reset();

    /// @brief Evicts slots whose lastUsedFrameId is older than (renderFrameId - idleFrames).
    ///        `idleFrames` must be >= framesInFlight so dropped textures are GPU-idle by
    ///        the time the underlying Ref releases.
    void EndFrame(std::uint64_t renderFrameId, std::uint32_t idleFrames);

    [[nodiscard]] std::size_t Size() const noexcept { return slots_.size(); }

private:
    struct Slot
    {
        Ref<RHI::IRHITexture> texture;
        RHI::TextureDesc      desc{};
        std::uint64_t         lastUsedFrameId{0};
    };

    [[nodiscard]] static bool DescEquals(const RHI::TextureDesc& a, const RHI::TextureDesc& b) noexcept;

    RHI::IRHIDevice*                            device_{nullptr};
    std::unordered_map<std::string, Slot>       slots_;
};

} // namespace Hylux::Renderer
