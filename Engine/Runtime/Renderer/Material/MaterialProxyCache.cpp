/// @file
/// @brief MaterialProxyCache implementation.

#include "Renderer/Material/MaterialProxyCache.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/IRHIDevice.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIResourceDesc.h"
#include "Shader/ShaderSubsystem.h"

#include <cstring>

namespace Hylux::Renderer
{
namespace
{

/// @brief Allocates a CpuToGpu uniform buffer of `size` bytes, copies @p bytes into it,
///        and returns the (buffer, bindless index) pair. On any failure returns
///        (null, 0xFFFFFFFF). Empty payloads short-circuit before touching RHI.
struct UniformBufferAllocation
{
    Ref<RHI::IRHIBuffer> buffer;
    std::uint32_t        bindlessIndex{0xFFFFFFFFu};
};

UniformBufferAllocation AllocateUniformBuffer(RHI::IRHIDevice& device, const void* bytes, std::size_t size)
{
    UniformBufferAllocation out{};
    if (size == 0 || bytes == nullptr)
    {
        return out;
    }

    RHI::BufferDesc desc{};
    desc.size        = size;
    desc.usage       = RHI::BufferUsage::UniformBuffer;
    desc.memory      = RHI::MemoryUsage::CpuToGpu;
    desc.bindlessCbv = true;

    out.buffer = device.CreateBuffer(desc);
    if (!out.buffer)
    {
        HYLUX_LOG_ERROR(LogRender, "MaterialProxyCache: uniform buffer alloc failed ({} bytes)", size);
        return {};
    }

    void* mapped = out.buffer->Map(0, size);
    if (mapped == nullptr)
    {
        HYLUX_LOG_ERROR(LogRender, "MaterialProxyCache: uniform buffer map failed");
        return {};
    }
    std::memcpy(mapped, bytes, size);
    out.buffer->Unmap();

    const auto idx = out.buffer->GetBindlessIndex(RHI::BindlessKind::SrvCbvUav);
    out.bindlessIndex =
        idx == RHI::BindlessIndex::Invalid ? 0xFFFFFFFFu : static_cast<std::uint32_t>(idx);
    return out;
}

} // namespace

MaterialProxyCache::MaterialProxyCache(RHI::IRHIDevice* device, Shader::ShaderSubsystem* shaders) noexcept
    : device_(device), shaders_(shaders)
{}

MaterialProxy* MaterialProxyCache::GetOrCreate(const MaterialSnapshot& snapshot)
{
    const Key key{snapshot.materialAssetHash, snapshot.instanceHash};
    if (auto it = entries_.find(key); it != entries_.end())
    {
        return it->second.get();
    }

    Shader::MaterialShaderMap* shaderMap = nullptr;
    if (shaders_ != nullptr && snapshot.materialAssetHash != 0)
    {
        shaderMap = shaders_->GetOrLoadMaterialShaderMap(snapshot.materialAssetHash);
    }

    UniformBufferAllocation uniform{};
    if (device_ != nullptr)
    {
        uniform = AllocateUniformBuffer(*device_, snapshot.uniformBlock.data(),
                                        snapshot.uniformBlock.size());
    }

    auto proxy = std::make_unique<MaterialProxy>(snapshot.materialAssetHash,
                                                 snapshot.instanceHash,
                                                 snapshot.permutationKey,
                                                 shaderMap,
                                                 uniform.bindlessIndex,
                                                 std::move(uniform.buffer),
                                                 snapshot.textureBindlessIndices,
                                                 snapshot.referencedTextures);

    auto* raw = proxy.get();
    entries_.emplace(key, std::move(proxy));
    return raw;
}

void MaterialProxyCache::Clear() noexcept
{
    entries_.clear();
}

} // namespace Hylux::Renderer
