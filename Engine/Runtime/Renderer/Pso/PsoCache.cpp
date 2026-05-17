/// @file
/// @brief PsoCache implementation. Resolves shaders through ShaderSubsystem, computes the
///        four-bucket PsoKey, and caches IRHIGraphicsPipeline instances.

#include "Renderer/Pso/PsoCache.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Core/Utils/Hash.h"
#include "RHI/IRHIDevice.h"
#include "RHI/RHIPipelineDesc.h"
#include "Renderer/Pso/PipelineLayoutCache.h"
#include "Renderer/Pso/PsoBlobStore.h"
#include "Shader/ShaderMap/MaterialShaderMap.h"
#include "Shader/ShaderRef.h"
#include "Shader/ShaderSubsystem.h"

#include <cstring>

namespace Hylux::Renderer
{
namespace
{

inline std::uint64_t HashBytes(std::uint64_t seed, const void* data, std::size_t size) noexcept
{
    return Hash::Fnv1a64(static_cast<const char*>(data), size, seed);
}

inline std::uint64_t HashRenderState(const PipelineRenderState& state) noexcept
{
    std::uint64_t h = Hash::Fnv1a64Offset;
    h = HashBytes(h, &state.topology,     sizeof(state.topology));
    h = HashBytes(h, &state.rasterizer,   sizeof(state.rasterizer));
    h = HashBytes(h, &state.depthStencil, sizeof(state.depthStencil));
    h = HashBytes(h, &state.blend,        sizeof(state.blend));
    for (const auto& binding : state.vertexInput.bindings)
    {
        h = HashBytes(h, &binding, sizeof(binding));
    }
    for (const auto& attribute : state.vertexInput.attributes)
    {
        h = HashBytes(h, &attribute, sizeof(attribute));
    }
    return h;
}

inline std::uint64_t HashFormatKey(const PsoFormatKey& key) noexcept
{
    return HashBytes(Hash::Fnv1a64Offset, &key, sizeof(key));
}

inline std::uint64_t HashShaderIds(const Shader::ShaderRef& vs, const Shader::ShaderRef& ps) noexcept
{
    std::uint64_t h = Hash::Fnv1a64Offset;
    h = HashBytes(h, &vs.id.value, sizeof(vs.id.value));
    h = HashBytes(h, &ps.id.value, sizeof(ps.id.value));
    return h;
}

/// @brief Dedup key for failure logging: hash of (pass, perm, materialAsset). One log
///        line per unique signature even though GetOrCreate is called every frame.
inline std::uint64_t HashFailureSignature(const PsoRequest& req) noexcept
{
    std::uint64_t h = Hash::Fnv1a64Offset;
    h = HashBytes(h, &req.passNameHash,      sizeof(req.passNameHash));
    h = HashBytes(h, &req.permutationKey,    sizeof(req.permutationKey));
    h = HashBytes(h, &req.materialAssetHash, sizeof(req.materialAssetHash));
    return h;
}

} // namespace

PsoCache::PsoCache(RHI::IRHIDevice*         device,
                   Shader::ShaderSubsystem* shaders,
                   PipelineLayoutCache*     layouts,
                   RHI::IRHIPipelineCache*  backendCache,
                   PsoBlobStore*            blobStore,
                   std::string              blobName)
    : device_(device),
      shaders_(shaders),
      layouts_(layouts),
      backendCache_(backendCache),
      blobStore_(blobStore),
      blobName_(std::move(blobName))
{
}

PsoHandle PsoCache::GetOrCreate(const PsoRequest& request)
{
    if (device_ == nullptr || shaders_ == nullptr || layouts_ == nullptr ||
        request.renderState == nullptr)
    {
        ++stats_.failureCount;
        return {};
    }

    Shader::ShaderRef vs{};
    Shader::ShaderRef ps{};
    if (request.materialAssetHash != 0)
    {
        Shader::MaterialShaderMap* shaderMap =
            shaders_->GetOrLoadMaterialShaderMap(request.materialAssetHash);
        if (shaderMap == nullptr)
        {
            HYLUX_LOG_WARN(LogRender, "PsoCache: material shader map missing (asset={:#x})",
                           request.materialAssetHash);
            ++stats_.failureCount;
            return {};
        }
        vs = shaderMap->Get(request.passNameHash, request.permutationKey, RHI::ShaderStage::Vertex);
        ps = shaderMap->Get(request.passNameHash, request.permutationKey, RHI::ShaderStage::Pixel);
    }
    else
    {
        vs = shaders_->GetPassShader(request.passNameHash, request.permutationKey,
                                     RHI::ShaderStage::Vertex);
        ps = shaders_->GetPassShader(request.passNameHash, request.permutationKey,
                                     RHI::ShaderStage::Pixel);
    }
    if (!vs || !ps || vs.reflection == nullptr)
    {
        const std::uint64_t sig = HashFailureSignature(request);
        if (loggedFailures_.insert(sig).second)
        {
            HYLUX_LOG_WARN(LogRender,
                           "PsoCache: shader resolution failed (pass={:#x}, perm={:#x}, "
                           "material={:#x}, vs={}, ps={}); further failures for the same "
                           "signature will be suppressed.",
                           request.passNameHash, request.permutationKey, request.materialAssetHash,
                           static_cast<bool>(vs), static_cast<bool>(ps));
        }
        ++stats_.failureCount;
        return {};
    }

    PsoKey key{};
    key.pipelineLayoutHash     = vs.reflection->pipelineLayoutHash;
    key.shaderIdsHash          = HashShaderIds(vs, ps);
    key.renderStateHash        = HashRenderState(*request.renderState);
    key.formatAndMultiviewHash = HashFormatKey(request.formats);

    if (auto it = entries_.find(key); it != entries_.end())
    {
        ++stats_.hitCount;
        return PsoHandle{it->second.pipeline.Get(), it->second.layout, it->second.pushConstantSize};
    }

    RHI::IRHIPipelineLayout* layout = layouts_->GetOrCreate(key.pipelineLayoutHash, *vs.reflection);
    if (layout == nullptr)
    {
        ++stats_.failureCount;
        return {};
    }

    RHI::GraphicsPipelineDesc desc{};
    desc.layout              = layout;
    desc.vertexShader        = vs.module;
    desc.pixelShader         = ps.module;
    desc.vertexInput         = request.renderState->vertexInput;
    desc.topology            = request.renderState->topology;
    desc.rasterizer          = request.renderState->rasterizer;
    desc.depthStencil        = request.renderState->depthStencil;
    desc.blend               = request.renderState->blend;
    desc.colorFormats        = request.formats.colorFormats;
    desc.depthStencilFormat  = request.formats.depthStencilFormat;
    desc.sampleCount         = request.formats.sampleCount;
    desc.viewMask            = request.formats.viewMask;
    desc.pipelineCache       = backendCache_;

    auto pipeline = device_->CreateGraphicsPipeline(desc);
    if (!pipeline)
    {
        HYLUX_LOG_ERROR(LogRender,
                        "PsoCache: CreateGraphicsPipeline failed (pass={:#x}, perm={:#x})",
                        request.passNameHash, request.permutationKey);
        ++stats_.failureCount;
        return {};
    }

    Entry entry{};
    entry.pipeline         = pipeline;
    entry.layout           = layout;
    entry.pushConstantSize = vs.reflection->pushConstantSize;
    auto [it, inserted]    = entries_.emplace(key, std::move(entry));
    ++stats_.missCount;
    return PsoHandle{it->second.pipeline.Get(), it->second.layout, it->second.pushConstantSize};
}

void PsoCache::InvalidateAll()
{
    entries_.clear();
    // After a shader archive reload the same passNameHash may now resolve (or fail in a
    // different way). Reset the failure dedup so the next miss logs again.
    loggedFailures_.clear();
}

void PsoCache::InvalidateByShader(std::span<const Shader::ShaderId> /*ids*/)
{
    // Stage 2: drop the entire map. Per-id pruning lands when DrawList stages need it.
    InvalidateAll();
}

void PsoCache::FlushBlobStore()
{
    if (backendCache_ == nullptr || blobStore_ == nullptr || blobName_.empty())
    {
        return;
    }
    auto blob = backendCache_->SerializeToBlob();
    if (blob.empty())
    {
        return;
    }
    if (!blobStore_->Save(blobName_, blob))
    {
        HYLUX_LOG_WARN(LogRender, "PsoCache: failed to persist pipeline cache blob '{}'", blobName_);
    }
}

void PsoCache::WarmFromBlobStore()
{
    if (backendCache_ == nullptr || blobStore_ == nullptr || blobName_.empty())
    {
        return;
    }
    auto blob = blobStore_->Load(blobName_);
    if (blob.empty())
    {
        return;
    }
    if (!backendCache_->LoadFromBlob(blob))
    {
        HYLUX_LOG_WARN(LogRender, "PsoCache: backend rejected pipeline cache blob '{}'", blobName_);
    }
}

} // namespace Hylux::Renderer
