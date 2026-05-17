/// @file
/// @brief Render-thread cache of MaterialProxy instances keyed by
///        (materialAssetHash, instanceHash). Two MaterialInstance objects sharing the same
///        asset and parameter values resolve to the same proxy, keeping PSO cache hit
///        rates high.

#pragma once

#include "Renderer/Material/MaterialProxy.h"
#include "Renderer/Material/MaterialSnapshot.h"
#include "Renderer/RendererForward.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace Hylux::RHI
{
class IRHIDevice;
}

namespace Hylux::Shader
{
class ShaderSubsystem;
}

namespace Hylux::Renderer
{

/// @brief Render-thread proxy cache. Allocates a per-(assetHash, instanceHash) uniform
///        buffer for the snapshot's uniform block and forwards the snapshot's
///        pre-resolved bindless indices into the MaterialProxy. Texture residency is
///        retained via Ref<TextureAsset> on the proxy so game-side MaterialInstance
///        drops cannot leave the GPU side with stale bindless slots.
class MaterialProxyCache
{
public:
    MaterialProxyCache(RHI::IRHIDevice* device, Shader::ShaderSubsystem* shaders) noexcept;

    MaterialProxyCache(const MaterialProxyCache&)            = delete;
    MaterialProxyCache& operator=(const MaterialProxyCache&) = delete;

    /// @brief Returns the proxy for the supplied snapshot, lazily building one on miss.
    ///        Returned pointer is stable until Clear is called.
    [[nodiscard]] MaterialProxy* GetOrCreate(const MaterialSnapshot& snapshot);

    /// @brief Drops every cached proxy. Called from RenderSubsystem::Shutdown.
    void Clear() noexcept;

    [[nodiscard]] std::size_t Size() const noexcept { return entries_.size(); }

private:
    struct Key
    {
        std::uint64_t assetHash{0};
        std::uint64_t instanceHash{0};

        [[nodiscard]] friend bool operator==(const Key&, const Key&) noexcept = default;
    };

    struct KeyHash
    {
        [[nodiscard]] std::size_t operator()(const Key& key) const noexcept
        {
            std::uint64_t h = key.assetHash ^ (key.instanceHash + 0x9E3779B97F4A7C15ull + (key.assetHash << 6) + (key.assetHash >> 2));
            return static_cast<std::size_t>(h);
        }
    };

    RHI::IRHIDevice*                                                 device_{nullptr};
    Shader::ShaderSubsystem*                                         shaders_{nullptr};
    std::unordered_map<Key, std::unique_ptr<MaterialProxy>, KeyHash> entries_;
};

} // namespace Hylux::Renderer
