/// @file
/// @brief RenderGraph orchestrator: owns the per-frame pass list, resource nodes, and the
///        Compile/Execute pipeline. Per-frame transient — construct on the stack each frame
///        from the host renderer.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/RHIBarriers.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"
#include "RHI/RHIResourceDesc.h"
#include "RHI/RHITypes.h"
#include "RenderGraph/Internal/RGNode.h"
#include "RenderGraph/Internal/RGResourceRegistry.h"
#include "RenderGraph/RGAccess.h"
#include "RenderGraph/RGBuilder.h"
#include "RenderGraph/RGHandle.h"
#include "RenderGraph/RGPass.h"
#include "RenderGraph/RGRasterBuilder.h"
#include "RenderGraph/RGRasterPass.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Hylux::RG
{

/// @brief Per-frame DAG of RenderGraph passes. Lifetime is one frame:
///        construct -> AddPass(s) -> Compile -> Execute. Not an ISubsystem; the future
///        RendererSubsystem owns / drives one (or pools them) per frame.
class RenderGraph
{
public:
    explicit RenderGraph(RHI::IRHIDevice* device);
    ~RenderGraph();

    RenderGraph(const RenderGraph&)            = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&)                 = delete;
    RenderGraph& operator=(RenderGraph&&)      = delete;

    /// @brief Constructs a TPass in-place, registers it, and immediately runs Setup with the
    ///        appropriate builder type (RGRasterBuilder if TPass derives from RGRasterPass,
    ///        otherwise RGBuilder). Returns a borrowed pointer; ownership stays with the graph.
    template<typename TPass, typename... Args>
    TPass* AddPass(std::string_view name, Args&&... args);

    /// @brief Performs dead-pass culling, topological sort, transient resource creation, and
    ///        per-pass barrier planning. Must be called before Execute.
    void Compile();

    /// @brief Records barriers, attachments, debug markers, and pass Execute calls onto the
    ///        supplied command list in compiled order.
    void Execute(RHI::IRHICommandList& cmd);

    [[nodiscard]] RHI::IRHIDevice* GetDevice() const noexcept { return device_; }

private:
    friend class RGBuilder;
    friend class RGRasterBuilder;

    [[nodiscard]] std::uint32_t CreateTextureNode(std::string_view name,
                                                  const RHI::TextureDesc& desc);
    [[nodiscard]] std::uint32_t ImportTextureNode(std::string_view name,
                                                  RHI::IRHITexture*       texture,
                                                  RHI::ImageLayout        initialLayout);
    void                        SetTextureFinalLayout(std::uint32_t textureIndex,
                                                      RHI::ImageLayout finalLayout);

    [[nodiscard]] std::uint32_t CreateBufferNode(std::string_view name,
                                                 const RHI::BufferDesc& desc);
    [[nodiscard]] std::uint32_t ImportBufferNode(std::string_view name,
                                                 RHI::IRHIBuffer* buffer);

    void RecordTextureRead(std::uint32_t passIndex, RGTextureHandle handle, RGTextureAccess access);
    [[nodiscard]] RGTextureHandle RecordTextureWrite(std::uint32_t passIndex,
                                                     RGTextureHandle handle,
                                                     RGTextureAccess access);

    void RecordBufferRead(std::uint32_t passIndex, RGBufferHandle handle, RGBufferAccess access);
    [[nodiscard]] RGBufferHandle RecordBufferWrite(std::uint32_t passIndex,
                                                   RGBufferHandle handle,
                                                   RGBufferAccess access);

    void MarkPassSideEffect(std::uint32_t passIndex);

    void RecordColorAttachment(std::uint32_t passIndex,
                               std::uint32_t slot,
                               RGTextureHandle handle,
                               RHI::LoadOp loadOp,
                               RHI::StoreOp storeOp,
                               RHI::ClearValue clearValue,
                               RGTextureHandle resolveHandle);
    void RecordDepthAttachment(std::uint32_t passIndex,
                               RGTextureHandle handle,
                               RHI::LoadOp loadOp,
                               RHI::StoreOp storeOp,
                               RHI::ClearValue clearValue);
    void RecordRenderArea(std::uint32_t passIndex, RHI::Rect2D area);

    void CullDeadPasses();
    void TopologicallySortPasses();
    void RealizeResources();
    void PlanBarriers();

    void AccumulateTextureUsage(Internal::RGTextureNode& node, const RGTextureAccess& access);
    void AccumulateBufferUsage(Internal::RGBufferNode& node, const RGBufferAccess& access);

    void RecordPass(RHI::IRHICommandList& cmd, std::uint32_t compiledIndex);

    RHI::IRHIDevice*                              device_{nullptr};
    std::vector<std::unique_ptr<RGPass>>          passOwners_;
    std::vector<Internal::RGPassNode>             passes_;
    std::vector<Internal::RGTextureNode>          textures_;
    std::vector<Internal::RGBufferNode>           buffers_;
    std::vector<std::uint32_t>                    executionOrder_;
    std::vector<std::vector<std::uint32_t>>       textureVersionProducers_;
    std::vector<std::vector<std::uint32_t>>       bufferVersionProducers_;
    std::vector<Internal::RGTextureState>         currentTextureState_;
    std::vector<Internal::RGBufferState>          currentBufferState_;
    std::unique_ptr<Internal::RGResourceRegistry> registry_;
    bool                                          compiled_{false};
};

template<typename TPass, typename... Args>
TPass* RenderGraph::AddPass(std::string_view name, Args&&... args)
{
    static_assert(std::is_base_of_v<RGPass, TPass>, "TPass must derive from RGPass");

    auto owned = std::make_unique<TPass>(std::forward<Args>(args)...);
    TPass* raw = owned.get();
    raw->name_ = std::string(name);

    const std::uint32_t passIndex = static_cast<std::uint32_t>(passes_.size());
    Internal::RGPassNode node;
    node.name     = std::string(name);
    node.pass     = raw;
    node.isRaster = std::is_base_of_v<RGRasterPass, TPass>;
    passes_.push_back(std::move(node));
    passOwners_.push_back(std::move(owned));

    if constexpr (std::is_base_of_v<RGRasterPass, TPass>)
    {
        RGRasterBuilder builder(this, passIndex);
        static_cast<RGRasterPass*>(raw)->Setup(static_cast<RGBuilder&>(builder));
    }
    else
    {
        RGBuilder builder(this, passIndex);
        raw->Setup(builder);
    }
    return raw;
}

} // namespace Hylux::RG
