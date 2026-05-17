/// @file
/// @brief Built draw list. Holds the GPU resources DrawListBuilder produced (instance
///        buffer, indirect args, indirect count) plus the descriptor used to drive
///        recording.

#pragma once

#include "RHI/RHIForward.h"
#include "Renderer/DrawList/DrawListDesc.h"
#include "Renderer/Proxy/ProxyId.h"

#include <cstdint>
#include <span>
#include <vector>

namespace Hylux::Renderer
{

/// @brief Opaque handle into DrawListBuilder's owning storage. Stable for the duration of
///        the frame that produced it.
struct DrawListHandle
{
    std::uint32_t index{0xFFFFFFFFu};

    [[nodiscard]] explicit operator bool() const noexcept { return index != 0xFFFFFFFFu; }
};

/// @brief Runtime draw list. Constructed by DrawListBuilder::Build; consumed by
///        RendererPassContext::DrawRendererList once the recorder lands.
class DrawList
{
public:
    DrawList() = default;
    explicit DrawList(DrawListDesc desc) noexcept;

    [[nodiscard]] const DrawListDesc& GetDesc() const noexcept { return desc_; }

    [[nodiscard]] std::uint32_t    GetDrawCount() const noexcept { return drawCount_; }
    [[nodiscard]] RHI::IRHIBuffer* GetIndirectArgs() const noexcept { return indirectArgs_; }
    [[nodiscard]] RHI::IRHIBuffer* GetIndirectCount() const noexcept { return indirectCount_; }
    [[nodiscard]] RHI::IRHIBuffer* GetInstanceBuffer() const noexcept { return instanceBuffer_; }
    [[nodiscard]] std::uint32_t    GetInstanceBindlessIndex() const noexcept { return instanceBindlessIndex_; }
    [[nodiscard]] std::uint32_t    GetTransformBindlessIndex() const noexcept { return transformBindlessIndex_; }

    /// @brief Byte offsets into the underlying upload-heap buffers. Required because
    ///        DrawListBuilder packs args / count into shared per-frame ring slots.
    [[nodiscard]] std::uint64_t GetIndirectArgsOffset() const noexcept { return indirectArgsOffset_; }
    [[nodiscard]] std::uint64_t GetIndirectCountOffset() const noexcept { return indirectCountOffset_; }
    [[nodiscard]] std::uint64_t GetInstanceBufferOffset() const noexcept { return instanceBufferOffset_; }

    /// @brief Surviving proxy ids after culling / filtering. Used by stage 4b+ indirect-args
    ///        generation; today consumers can read it directly for debug overlays.
    [[nodiscard]] std::span<const ProxyId> GetSurvivors() const noexcept { return survivors_; }

    /// @brief Builder-side mutator. Public so DrawListBuilder can populate without
    ///        introducing a friend declaration; the field is not modified after Build.
    void SetCollected(std::uint32_t drawCount, RHI::IRHIBuffer* instanceBuffer, std::uint32_t instanceBindlessIndex,
                      RHI::IRHIBuffer* indirectArgs, RHI::IRHIBuffer* indirectCount,
                      std::uint32_t transformBindlessIndex) noexcept;

    /// @brief Builder-side mutator for the byte offsets matching the buffers from
    ///        SetCollected. Defaults to 0 if not called.
    void SetBufferOffsets(const std::uint64_t indirectArgsOffset, const std::uint64_t indirectCountOffset,
                          const std::uint64_t instanceBufferOffset) noexcept
    {
        indirectArgsOffset_ = indirectArgsOffset;
        indirectCountOffset_ = indirectCountOffset;
        instanceBufferOffset_ = instanceBufferOffset;
    }

    /// @brief Builder-side mutator. Stores the surviving proxy id list.
    void SetSurvivors(std::vector<ProxyId> survivors) noexcept { survivors_ = std::move(survivors); }

private:
    DrawListDesc         desc_{};
    std::uint32_t        drawCount_{0};
    RHI::IRHIBuffer*     indirectArgs_{nullptr};
    RHI::IRHIBuffer*     indirectCount_{nullptr};
    RHI::IRHIBuffer*     instanceBuffer_{nullptr};
    std::uint32_t        instanceBindlessIndex_{0xFFFFFFFFu};
    std::uint32_t        transformBindlessIndex_{0xFFFFFFFFu};
    std::uint64_t        indirectArgsOffset_{0};
    std::uint64_t        indirectCountOffset_{0};
    std::uint64_t        instanceBufferOffset_{0};
    std::vector<ProxyId> survivors_{};
};

} // namespace Hylux::Renderer
