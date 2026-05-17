/// @file
/// @brief AssetSubsystem implementation + the private AssetUploader that stages CPU
///        bytes into GPU buffers and textures via a dedicated graphics-queue command
///        pool and a one-shot fence. v1 blocks on the fence; v2 will route this through
///        a transfer queue + worker thread without changing IAssetUploader.

#include "Asset/AssetSubsystem.h"

#include "Asset/AssetLoaderContext.h"
#include "Asset/Cooked/CookedReader.h"
#include "Asset/Loaders/MaterialInstanceLoader.h"
#include "Asset/Loaders/MaterialLoader.h"
#include "Asset/Loaders/MeshLoader.h"
#include "Asset/Loaders/TextureLoader.h"
#include "Core/IO/Virtual/VirtualFileSystem.h"
#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "Engine/Engine.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/IRHICommandList.h"
#include "RHI/IRHICommandPool.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHIFence.h"
#include "RHI/IRHIQueue.h"
#include "RHI/IRHITexture.h"
#include "RHI/RHIBarriers.h"
#include "RHI/RHIResourceDesc.h"
#include "RHI/RHISubsystem.h"
#include "Renderer/Subsystem/RenderSubsystem.h"
#include "Shader/ShaderSubsystem.h"

#include <array>
#include <cstring>
#include <span>
#include <utility>
#include <vector>

namespace Hylux::Asset
{

namespace
{

class AssetUploader final : public IAssetUploader
{
public:
    AssetUploader(RHI::IRHIDevice* device, Ref<RHI::IRHIQueue> queue, Ref<RHI::IRHICommandPool> pool,
                  Ref<RHI::IRHIFence> fence) noexcept
        : device_(device), queue_(std::move(queue)), pool_(std::move(pool)), fence_(std::move(fence))
    {}

    bool UploadBufferData(RHI::IRHIBuffer& destination, const void* bytes, std::uint64_t size) override
    {
        if (size == 0 || bytes == nullptr)
        {
            return true;
        }
        Ref<RHI::IRHIBuffer> staging = AllocateStaging(size);
        if (!staging)
        {
            return false;
        }
        if (!WriteToMapped(*staging, bytes, size))
        {
            return false;
        }

        Ref<RHI::IRHICommandList> cmd;
        if (!BeginOneShot(cmd))
        {
            return false;
        }
        cmd->CopyBuffer(&destination, 0, staging.Get(), 0, size);
        return EndAndWait(*cmd);
    }

    bool UploadTextureMips(RHI::IRHITexture& destination, std::span<const MipUpload> mips) override
    {
        if (mips.empty())
        {
            return true;
        }

        std::uint64_t totalBytes = 0;
        for (const auto& m : mips)
        {
            totalBytes += m.bytes.size();
        }
        if (totalBytes == 0)
        {
            return true;
        }

        Ref<RHI::IRHIBuffer> staging = AllocateStaging(totalBytes);
        if (!staging)
        {
            return false;
        }

        void* mapped = staging->Map(0, totalBytes);
        if (mapped == nullptr)
        {
            return false;
        }
        std::vector<std::uint64_t> mipOffsets;
        mipOffsets.reserve(mips.size());
        std::uint64_t cursor = 0;
        for (const auto& m : mips)
        {
            mipOffsets.push_back(cursor);
            std::memcpy(static_cast<std::byte*>(mapped) + cursor, m.bytes.data(), m.bytes.size());
            cursor += m.bytes.size();
        }
        staging->Unmap();

        std::uint32_t maxMipLevel    = 0;
        std::uint32_t maxArrayLayer  = 0;
        for (const auto& m : mips)
        {
            if (m.mipLevel > maxMipLevel)       maxMipLevel = m.mipLevel;
            if (m.arrayLayer > maxArrayLayer)   maxArrayLayer = m.arrayLayer;
        }

        Ref<RHI::IRHICommandList> cmd;
        if (!BeginOneShot(cmd))
        {
            return false;
        }

        RHI::TextureBarrier toTransfer{};
        toTransfer.texture                  = &destination;
        toTransfer.range.baseMipLevel       = 0;
        toTransfer.range.mipLevelCount      = maxMipLevel + 1;
        toTransfer.range.baseArrayLayer     = 0;
        toTransfer.range.arrayLayerCount    = maxArrayLayer + 1;
        toTransfer.oldLayout                = RHI::ImageLayout::Undefined;
        toTransfer.newLayout                = RHI::ImageLayout::TransferDst;
        toTransfer.srcStages                = RHI::PipelineStageMask::TopOfPipe;
        toTransfer.srcAccess                = RHI::AccessMask::None;
        toTransfer.dstStages                = RHI::PipelineStageMask::Transfer;
        toTransfer.dstAccess                = RHI::AccessMask::TransferWrite;
        std::array              toTransferArr{toTransfer};
        RHI::BarrierGroup       prep{};
        prep.textures = std::span<const RHI::TextureBarrier>{toTransferArr};
        cmd->Barrier(prep);

        for (std::size_t i = 0; i < mips.size(); ++i)
        {
            const auto&         m = mips[i];
            RHI::TextureRegion  region{};
            region.range.baseMipLevel    = m.mipLevel;
            region.range.mipLevelCount   = 1;
            region.range.baseArrayLayer  = m.arrayLayer;
            region.range.arrayLayerCount = 1;
            region.offset                = {};
            region.extent.width          = m.width;
            region.extent.height         = m.height;
            region.extent.depth          = 1;

            RHI::BufferTextureLayout layout{};
            layout.offset     = mipOffsets[i];
            layout.rowPitch   = 0;
            layout.slicePitch = 0;

            cmd->CopyBufferToTexture(&destination, region, staging.Get(), layout);
        }

        RHI::TextureBarrier toShader{};
        toShader.texture               = &destination;
        toShader.range                 = toTransfer.range;
        toShader.oldLayout             = RHI::ImageLayout::TransferDst;
        toShader.newLayout             = RHI::ImageLayout::ShaderReadOnly;
        toShader.srcStages             = RHI::PipelineStageMask::Transfer;
        toShader.srcAccess             = RHI::AccessMask::TransferWrite;
        toShader.dstStages             = RHI::PipelineStageMask::AllCommands;
        toShader.dstAccess             = RHI::AccessMask::ShaderResourceRead;
        std::array              toShaderArr{toShader};
        RHI::BarrierGroup       finish{};
        finish.textures = std::span<const RHI::TextureBarrier>{toShaderArr};
        cmd->Barrier(finish);

        return EndAndWait(*cmd);
    }

private:
    Ref<RHI::IRHIBuffer> AllocateStaging(std::uint64_t size)
    {
        if (device_ == nullptr)
        {
            return {};
        }
        RHI::BufferDesc desc{};
        desc.size   = size;
        desc.usage  = RHI::BufferUsage::TransferSrc;
        desc.memory = RHI::MemoryUsage::CpuToGpu;
        auto buf    = device_->CreateBuffer(desc);
        if (!buf)
        {
            HYLUX_LOG_ERROR(LogAsset, "AssetUploader: staging allocation of {} bytes failed", size);
        }
        return buf;
    }

    bool WriteToMapped(RHI::IRHIBuffer& staging, const void* bytes, std::uint64_t size)
    {
        void* mapped = staging.Map(0, size);
        if (mapped == nullptr)
        {
            HYLUX_LOG_ERROR(LogAsset, "AssetUploader: staging map failed");
            return false;
        }
        std::memcpy(mapped, bytes, size);
        staging.Unmap();
        return true;
    }

    bool BeginOneShot(Ref<RHI::IRHICommandList>& outCmd)
    {
        if (!pool_)
        {
            return false;
        }
        outCmd = pool_->AllocateCommandList();
        if (!outCmd)
        {
            HYLUX_LOG_ERROR(LogAsset, "AssetUploader: AllocateCommandList failed");
            return false;
        }
        if (!outCmd->Begin())
        {
            HYLUX_LOG_ERROR(LogAsset, "AssetUploader: command list Begin failed");
            return false;
        }
        return true;
    }

    bool EndAndWait(RHI::IRHICommandList& cmd)
    {
        cmd.End();

        ++fenceValue_;
        RHI::FenceSignalDesc signal{};
        signal.fence     = fence_.Get();
        signal.value     = fenceValue_;
        signal.stageMask = RHI::PipelineStageMask::AllCommands;

        std::array       cmdArr{&cmd};
        RHI::SubmitDesc  submit{};
        submit.commandLists = std::span<RHI::IRHICommandList* const>{cmdArr};
        submit.signals      = std::span<const RHI::FenceSignalDesc>{&signal, 1};

        std::array submits{submit};
        if (!queue_->Submit(std::span<const RHI::SubmitDesc>{submits}))
        {
            HYLUX_LOG_ERROR(LogAsset, "AssetUploader: queue submit failed");
            return false;
        }
        return fence_->Wait(fenceValue_);
    }

    RHI::IRHIDevice*          device_;
    Ref<RHI::IRHIQueue>       queue_;
    Ref<RHI::IRHICommandPool> pool_;
    Ref<RHI::IRHIFence>       fence_;
    std::uint64_t             fenceValue_{0};
};

} // namespace

AssetSubsystem::AssetSubsystem(AssetSubsystemConfig cfg)
    : config_(std::move(cfg)), lru_(config_.lruBudgetBytes)
{}

AssetSubsystem::~AssetSubsystem() = default;

std::vector<TypeId> AssetSubsystem::GetDependencies() const
{
    return {
        TypeIdOf<VirtualFileSystem>(),
        TypeIdOf<RHI::RHISubsystem>(),
        TypeIdOf<Shader::ShaderSubsystem>(),
        TypeIdOf<Renderer::RenderSubsystem>(),
    };
}

void AssetSubsystem::Initialize(Engine& engine)
{
    engine_   = &engine;
    vfs_      = engine.GetSubsystem<VirtualFileSystem>();
    rhi_      = engine.GetSubsystem<RHI::RHISubsystem>();
    shaders_  = engine.GetSubsystem<Shader::ShaderSubsystem>();
    renderer_ = engine.GetSubsystem<Renderer::RenderSubsystem>();
    if (vfs_ == nullptr || rhi_ == nullptr || shaders_ == nullptr || renderer_ == nullptr)
    {
        HYLUX_LOG_FATAL(LogAsset,
                        "AssetSubsystem: dependency missing (vfs={}, rhi={}, shaders={}, renderer={})",
                        vfs_ != nullptr, rhi_ != nullptr, shaders_ != nullptr, renderer_ != nullptr);
        return;
    }

    device_ = rhi_->GetDevice();
    if (device_ == nullptr)
    {
        HYLUX_LOG_FATAL(LogAsset, "AssetSubsystem: RHISubsystem returned a null device");
        return;
    }

    auto queue = device_->GetQueue(RHI::QueueType::Graphics, 0);
    auto pool  = device_->CreateCommandPool(RHI::QueueType::Graphics,
                                            RHI::CommandPoolFlagBits::AllowIndividualReset);
    auto fence = device_->CreateFence(0);
    if (!queue || !pool || !fence)
    {
        HYLUX_LOG_FATAL(LogAsset, "AssetSubsystem: failed to create upload primitives "
                                  "(queue={}, pool={}, fence={})",
                        static_cast<bool>(queue), static_cast<bool>(pool), static_cast<bool>(fence));
        return;
    }
    uploader_ = std::make_unique<AssetUploader>(device_, std::move(queue), std::move(pool), std::move(fence));

    if (config_.scanEngineMountAtInit)
    {
        registry_.Scan(*vfs_, "/Engine/");
    }
    if (config_.scanGameMountAtInit)
    {
        registry_.Scan(*vfs_, "/Game/");
    }
    const auto counts = registry_.ComputeTypeCounts();
    HYLUX_LOG_INFO(LogAsset,
                   "AssetSubsystem: registered {} assets ({} mesh, {} mat, {} mati, {} tex, {} unknown)",
                   registry_.Size(), counts.mesh, counts.material, counts.materialInstance, counts.texture,
                   counts.unknown);

    initialized_ = true;
}

void AssetSubsystem::Shutdown()
{
    if (!initialized_)
    {
        return;
    }
    HYLUX_LOG_INFO(LogAsset, "AssetSubsystem: shutting down ({} cached, {} bytes)",
                   lru_.Size(), lru_.BytesInUse());
    lru_.Clear();
    registry_.Clear();
    uploader_.reset();
    device_      = nullptr;
    renderer_    = nullptr;
    shaders_     = nullptr;
    rhi_         = nullptr;
    vfs_         = nullptr;
    engine_      = nullptr;
    initialized_ = false;
}

void AssetSubsystem::UnloadAll()
{
    lru_.Clear();
}

Future<Ref<AssetBase>> AssetSubsystem::LoadGenericByPath(std::string_view virtualPath)
{
    auto entry = registry_.FindByPath(virtualPath);
    if (!entry.has_value())
    {
        HYLUX_LOG_WARN(LogAsset, "AssetSubsystem: no registry entry for '{}'", virtualPath);
        return Future<Ref<AssetBase>>::MakeFailed();
    }
    return LoadGenericByGuid(entry->guid);
}

Future<Ref<AssetBase>> AssetSubsystem::LoadGenericByGuid(Guid guid)
{
    if (Ref<AssetBase> cached = lru_.Get(guid))
    {
        return Future<Ref<AssetBase>>::MakeReady(std::move(cached));
    }
    auto entry = registry_.Find(guid);
    if (!entry.has_value())
    {
        HYLUX_LOG_WARN(LogAsset, "AssetSubsystem: GUID {} not in registry", guid.ToString());
        return Future<Ref<AssetBase>>::MakeFailed();
    }
    Ref<AssetBase> built = ParseAndConstruct(*entry);
    if (!built)
    {
        return Future<Ref<AssetBase>>::MakeFailed();
    }
    const std::uint64_t footprint = built->GetMemoryFootprint();
    lru_.Insert(guid, built, footprint);
    return Future<Ref<AssetBase>>::MakeReady(std::move(built));
}

Ref<AssetBase> AssetSubsystem::ParseAndConstruct(const RegistryEntry& entry)
{
    auto reader = Cooked::CookedReader::Open(*vfs_, entry.path);
    if (!reader.has_value())
    {
        return {};
    }

    AssetLoaderContext ctx{};
    ctx.vfs       = vfs_;
    ctx.device    = device_;
    ctx.shaders   = shaders_;
    ctx.uploader  = uploader_.get();
    ctx.subsystem = this;
    ctx.registry  = &registry_;

    switch (entry.type)
    {
        case AssetTypeId::Mesh:
        {
            Ref<MeshAsset> mesh = MeshLoader::Load(*reader, ctx);
            return Ref<AssetBase>(mesh.Get());
        }
        case AssetTypeId::Material:
        {
            Ref<MaterialAsset> mat = MaterialLoader::Load(*reader, ctx);
            return Ref<AssetBase>(mat.Get());
        }
        case AssetTypeId::MaterialInstance:
        {
            Ref<MaterialInstanceAsset> mi = MaterialInstanceLoader::Load(*reader, ctx);
            return Ref<AssetBase>(mi.Get());
        }
        case AssetTypeId::Texture:
        {
            Ref<TextureAsset> tex = TextureLoader::Load(*reader, ctx);
            return Ref<AssetBase>(tex.Get());
        }
        case AssetTypeId::Unknown:
        default:
            HYLUX_LOG_WARN(LogAsset, "AssetSubsystem: unknown type tag for '{}'", entry.path);
            return {};
    }
}

} // namespace Hylux::Asset
