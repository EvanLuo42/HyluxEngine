/// @file
/// @brief Process-wide cache of GPU textures and buffers keyed by their TextureDesc /
///        BufferDesc. Replaces RG's per-frame CreateTexture / CreateBuffer calls with
///        a reuse-or-create lookup whose hit path skips the driver entirely.
///
///        Each entry stamps the frame it was last handed out. An entry is reusable when
///        its stamp + framesInFlight <= currentFrame (the GPU has finished with the
///        previous submission that borrowed it). The same Acquire-this-frame entry is
///        therefore not handed out twice in the same frame, so views inside one frame
///        get distinct physical resources and do not race on the shared command list.

#pragma once

#include "Core/Memory/Ref.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIResourceDesc.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace Hylux::RG::Internal
{

/// @brief Transient resource pool. Single-threaded (matches the renderer thread that
///        owns it). Factories are injected for test isolation; production wires them to
///        IRHIDevice::CreateTexture / CreateBuffer.
class RGTransientResourcePool
{
public:
    using TextureFactory = std::function<Ref<RHI::IRHITexture>(const RHI::TextureDesc&)>;
    using BufferFactory  = std::function<Ref<RHI::IRHIBuffer>(const RHI::BufferDesc&)>;

    /// @brief framesInFlight is the renderer's CPU-GPU lag (typically the swapchain
    ///        image count). Entries borrowed less than that many frames ago are
    ///        considered GPU-busy and not eligible for reuse.
    RGTransientResourcePool(TextureFactory textureFactory, BufferFactory bufferFactory,
                            std::uint32_t framesInFlight);
    ~RGTransientResourcePool();

    RGTransientResourcePool(const RGTransientResourcePool&)            = delete;
    RGTransientResourcePool& operator=(const RGTransientResourcePool&) = delete;
    RGTransientResourcePool(RGTransientResourcePool&&)                 = delete;
    RGTransientResourcePool& operator=(RGTransientResourcePool&&)      = delete;

    /// @brief Returns a Ref to a texture matching `desc`. Reuses an idle cached entry
    ///        when one exists; otherwise invokes the factory and caches the result.
    ///        Returns a null Ref only when the factory fails.
    [[nodiscard]] Ref<RHI::IRHITexture> AcquireTexture(const RHI::TextureDesc& desc,
                                                       std::uint64_t           frameId);

    /// @brief Buffer counterpart of AcquireTexture.
    [[nodiscard]] Ref<RHI::IRHIBuffer> AcquireBuffer(const RHI::BufferDesc& desc,
                                                     std::uint64_t          frameId);

    /// @brief Drops entries whose lastUsedFrame is older than (frameId - maxIdleFrames).
    ///        Caller must ensure the corresponding GPU work has completed; the typical
    ///        call site is the render thread right after waiting on the previous
    ///        frame's fence. maxIdleFrames should be >= framesInFlight.
    void EndFrame(std::uint64_t frameId, std::uint32_t maxIdleFrames);

    /// @brief Drops every cached entry unconditionally. Used in Shutdown after WaitIdle.
    void Reset();

    struct Stats
    {
        std::size_t textureEntries{0};
        std::size_t textureCreateCount{0};
        std::size_t textureReuseCount{0};
        std::size_t bufferEntries{0};
        std::size_t bufferCreateCount{0};
        std::size_t bufferReuseCount{0};
    };
    [[nodiscard]] Stats GetStats() const noexcept;

    [[nodiscard]] std::uint32_t GetFramesInFlight() const noexcept { return framesInFlight_; }

    /// @brief Field-wise structural equality on the two desc types. Public so tests and
    ///        diagnostics can assert pool keying behaviour.
    [[nodiscard]] static bool DescEquals(const RHI::TextureDesc& a, const RHI::TextureDesc& b) noexcept;
    [[nodiscard]] static bool DescEquals(const RHI::BufferDesc&  a, const RHI::BufferDesc&  b) noexcept;

private:
    struct TextureEntry
    {
        Ref<RHI::IRHITexture> texture;
        RHI::TextureDesc      desc{};
        std::uint64_t         lastUsedFrame{0};
    };

    struct BufferEntry
    {
        Ref<RHI::IRHIBuffer> buffer;
        RHI::BufferDesc      desc{};
        std::uint64_t        lastUsedFrame{0};
    };

    [[nodiscard]] bool IsReusable(std::uint64_t lastUsedFrame, std::uint64_t currentFrame) const noexcept;

    TextureFactory            textureFactory_;
    BufferFactory             bufferFactory_;
    std::uint32_t             framesInFlight_{1};
    std::vector<TextureEntry> textures_;
    std::vector<BufferEntry>  buffers_;
    Stats                     stats_{};
};

} // namespace Hylux::RG::Internal
