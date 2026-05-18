/// @file
/// @brief AssetSubsystem implementation + the private AssetUploader that stages CPU
///        bytes into GPU buffers and textures. The uploader owns a dedicated worker
///        thread + command pool: caller threads allocate the staging buffer and memcpy
///        the source bytes (so the source can be released immediately), then enqueue a
///        record-lambda; the worker drains the queue, records into its cmd buffer,
///        submits on the graphics queue with a fence signal, waits the fence, and sets
///        the Promise. Cancellation tokens are polled before the worker starts a job.

#include "Asset/AssetSubsystem.h"

#include "Asset/AssetLoaderContext.h"
#include "Asset/Cooked/CookedReader.h"
#include "Asset/Loaders/MaterialInstanceLoader.h"
#include "Asset/Loaders/MaterialLoader.h"
#include "Asset/Loaders/MeshLoader.h"
#include "Asset/Loaders/TextureLoader.h"
#include "Core/Async/Future.h"
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
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <functional>
#include <mutex>
#include <span>
#include <thread>
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
                  Ref<RHI::IRHIFence> fence)
        : device_(device), queue_(std::move(queue)), pool_(std::move(pool)), fence_(std::move(fence))
    {
        worker_ = std::thread([this] { WorkerLoop(); });
    }

    ~AssetUploader() override
    {
        {
            std::lock_guard lock(jobsMutex_);
            stop_.store(true, std::memory_order_release);
        }
        jobsCv_.notify_all();
        if (worker_.joinable())
        {
            worker_.join();
        }
        DrainPendingAsFailed();
    }

    AssetUploader(const AssetUploader&)            = delete;
    AssetUploader& operator=(const AssetUploader&) = delete;

    Future<bool> UploadBufferData(RHI::IRHIBuffer&  destination,
                                  const void*       bytes,
                                  std::uint64_t     size,
                                  CancellationToken token) override
    {
        Promise<bool> promise;
        Future<bool>  future = promise.GetFuture();
        if (size == 0 || bytes == nullptr)
        {
            promise.Set(true);
            return future;
        }

        Ref<RHI::IRHIBuffer> staging = AllocateStaging(size);
        if (!staging || !WriteToMapped(*staging, bytes, size))
        {
            promise.Set(false);
            return future;
        }

        Ref<RHI::IRHIBuffer> dstKeep(&destination);
        RHI::IRHIBuffer*     dst  = &destination;
        RHI::IRHIBuffer*     src  = staging.Get();

        UploadJob job;
        job.promise = std::move(promise);
        job.token   = std::move(token);
        job.bufferRefs.push_back(std::move(staging));
        job.bufferRefs.push_back(std::move(dstKeep));
        job.record = [dst, src, size](RHI::IRHICommandList& cmd) {
            cmd.CopyBuffer(dst, 0, src, 0, size);
        };
        Enqueue(std::move(job));
        return future;
    }

    Future<bool> UploadBufferRange(RHI::IRHIBuffer& /*destination*/, std::uint64_t /*dstOffset*/,
                                   const void* /*bytes*/, std::uint64_t /*size*/,
                                   CancellationToken /*token*/) override
    {
        HYLUX_LOG_WARN(LogAsset, "AssetUploader: UploadBufferRange not supported in v1");
        return Future<bool>::MakeReady(false);
    }

    Future<bool> UploadTextureTiles(RHI::IRHITexture& /*destination*/,
                                    std::span<const TileUpload> /*tiles*/,
                                    CancellationToken /*token*/) override
    {
        HYLUX_LOG_WARN(LogAsset, "AssetUploader: UploadTextureTiles not supported in v1 "
                                 "(VirtualTextureAsset backend not yet implemented)");
        return Future<bool>::MakeReady(false);
    }

    Future<bool> UploadTextureMips(RHI::IRHITexture&          destination,
                                   std::span<const MipUpload>  mips,
                                   CancellationToken          token) override
    {
        Promise<bool> promise;
        Future<bool>  future = promise.GetFuture();
        if (mips.empty())
        {
            promise.Set(true);
            return future;
        }

        std::uint64_t totalBytes = 0;
        for (const auto& m : mips)
        {
            totalBytes += m.bytes.size();
        }
        if (totalBytes == 0)
        {
            promise.Set(true);
            return future;
        }

        Ref<RHI::IRHIBuffer> staging = AllocateStaging(totalBytes);
        if (!staging)
        {
            promise.Set(false);
            return future;
        }
        void* mapped = staging->Map(0, totalBytes);
        if (mapped == nullptr)
        {
            HYLUX_LOG_ERROR(LogAsset, "AssetUploader: staging map failed");
            promise.Set(false);
            return future;
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

        std::vector<MipUpload> mipsCopy(mips.begin(), mips.end());
        for (auto& m : mipsCopy)
        {
            m.bytes = {};
        }

        std::uint32_t maxMipLevel   = 0;
        std::uint32_t maxArrayLayer = 0;
        for (const auto& m : mipsCopy)
        {
            if (m.mipLevel > maxMipLevel)       maxMipLevel = m.mipLevel;
            if (m.arrayLayer > maxArrayLayer)   maxArrayLayer = m.arrayLayer;
        }

        Ref<RHI::IRHITexture> dstKeep(&destination);
        RHI::IRHITexture*     dst = &destination;
        RHI::IRHIBuffer*      src = staging.Get();

        UploadJob job;
        job.promise = std::move(promise);
        job.token   = std::move(token);
        job.bufferRefs.push_back(std::move(staging));
        job.textureRefs.push_back(std::move(dstKeep));
        job.record = [dst, src, mipsCopy = std::move(mipsCopy), mipOffsets = std::move(mipOffsets),
                      maxMipLevel, maxArrayLayer](RHI::IRHICommandList& cmd) {
            RHI::TextureBarrier toTransfer{};
            toTransfer.texture               = dst;
            toTransfer.range.baseMipLevel    = 0;
            toTransfer.range.mipLevelCount   = maxMipLevel + 1;
            toTransfer.range.baseArrayLayer  = 0;
            toTransfer.range.arrayLayerCount = maxArrayLayer + 1;
            toTransfer.oldLayout             = RHI::ImageLayout::Undefined;
            toTransfer.newLayout             = RHI::ImageLayout::TransferDst;
            toTransfer.srcStages             = RHI::PipelineStageMask::TopOfPipe;
            toTransfer.srcAccess             = RHI::AccessMask::None;
            toTransfer.dstStages             = RHI::PipelineStageMask::Transfer;
            toTransfer.dstAccess             = RHI::AccessMask::TransferWrite;
            std::array        toTransferArr{toTransfer};
            RHI::BarrierGroup prep{};
            prep.textures = std::span<const RHI::TextureBarrier>{toTransferArr};
            cmd.Barrier(prep);

            for (std::size_t i = 0; i < mipsCopy.size(); ++i)
            {
                const auto&        m = mipsCopy[i];
                RHI::TextureRegion region{};
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

                cmd.CopyBufferToTexture(dst, region, src, layout);
            }

            RHI::TextureBarrier toShader{};
            toShader.texture   = dst;
            toShader.range     = toTransfer.range;
            toShader.oldLayout = RHI::ImageLayout::TransferDst;
            toShader.newLayout = RHI::ImageLayout::ShaderReadOnly;
            toShader.srcStages = RHI::PipelineStageMask::Transfer;
            toShader.srcAccess = RHI::AccessMask::TransferWrite;
            toShader.dstStages = RHI::PipelineStageMask::AllCommands;
            toShader.dstAccess = RHI::AccessMask::ShaderResourceRead;
            std::array        toShaderArr{toShader};
            RHI::BarrierGroup finish{};
            finish.textures = std::span<const RHI::TextureBarrier>{toShaderArr};
            cmd.Barrier(finish);
        };
        Enqueue(std::move(job));
        return future;
    }

private:
    struct UploadJob
    {
        std::function<void(RHI::IRHICommandList&)> record;
        Promise<bool>                              promise;
        CancellationToken                          token;
        std::vector<Ref<RHI::IRHIBuffer>>          bufferRefs;
        std::vector<Ref<RHI::IRHITexture>>         textureRefs;
    };

    void Enqueue(UploadJob job)
    {
        {
            std::lock_guard lock(jobsMutex_);
            jobs_.push_back(std::move(job));
        }
        jobsCv_.notify_one();
    }

    void WorkerLoop()
    {
        while (true)
        {
            UploadJob job;
            {
                std::unique_lock lock(jobsMutex_);
                jobsCv_.wait(lock, [this] {
                    return stop_.load(std::memory_order_acquire) || !jobs_.empty();
                });
                if (jobs_.empty() && stop_.load(std::memory_order_acquire))
                {
                    return;
                }
                job = std::move(jobs_.front());
                jobs_.pop_front();
            }
            RunJob(std::move(job));
        }
    }

    void RunJob(UploadJob job)
    {
        if (job.token.IsCanceled())
        {
            job.promise.Set(false);
            return;
        }
        if (!pool_)
        {
            job.promise.Set(false);
            return;
        }
        Ref<RHI::IRHICommandList> cmd = pool_->AllocateCommandList();
        if (!cmd || !cmd->Begin())
        {
            HYLUX_LOG_ERROR(LogAsset, "AssetUploader: command list begin failed");
            job.promise.Set(false);
            return;
        }
        job.record(*cmd);
        cmd->End();

        ++fenceValue_;
        RHI::FenceSignalDesc signal{};
        signal.fence     = fence_.Get();
        signal.value     = fenceValue_;
        signal.stageMask = RHI::PipelineStageMask::AllCommands;

        std::array      cmdArr{cmd.Get()};
        RHI::SubmitDesc submit{};
        submit.commandLists = std::span<RHI::IRHICommandList* const>{cmdArr};
        submit.signals      = std::span<const RHI::FenceSignalDesc>{&signal, 1};

        std::array submits{submit};
        if (!queue_->Submit(std::span<const RHI::SubmitDesc>{submits}))
        {
            HYLUX_LOG_ERROR(LogAsset, "AssetUploader: queue submit failed");
            job.promise.Set(false);
            return;
        }
        job.promise.Set(fence_->Wait(fenceValue_));
    }

    void DrainPendingAsFailed()
    {
        std::deque<UploadJob> leftover;
        {
            std::lock_guard lock(jobsMutex_);
            leftover.swap(jobs_);
        }
        for (auto& job : leftover)
        {
            job.promise.Set(false);
        }
    }

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

    RHI::IRHIDevice*          device_;
    Ref<RHI::IRHIQueue>       queue_;
    Ref<RHI::IRHICommandPool> pool_;
    Ref<RHI::IRHIFence>       fence_;
    std::uint64_t             fenceValue_{0};

    std::mutex              jobsMutex_;
    std::condition_variable jobsCv_;
    std::deque<UploadJob>   jobs_;
    std::atomic<bool>       stop_{false};
    std::thread             worker_;
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

    engine.RegisterTickable(this);
    initialized_ = true;
}

void AssetSubsystem::Tick(float /*deltaSeconds*/)
{
    tickExecutor_.Drain();
}

void AssetSubsystem::Shutdown()
{
    if (!initialized_)
    {
        return;
    }
    HYLUX_LOG_INFO(LogAsset, "AssetSubsystem: shutting down ({} cached, {} bytes)",
                   lru_.Size(), lru_.BytesInUse());

    if (engine_ != nullptr)
    {
        engine_->UnregisterTickable(this);
    }

    uploader_.reset();
    tickExecutor_.Drain();

    {
        std::lock_guard lock(stateMutex_);
        inflight_.clear();
        lru_.Clear();
    }
    registry_.Clear();
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
    std::lock_guard lock(stateMutex_);
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
    Promise<Ref<AssetBase>> ownedPromise;
    Future<Ref<AssetBase>>  ownedFuture = ownedPromise.GetFuture();
    {
        std::lock_guard lock(stateMutex_);
        if (Ref<AssetBase> cached = lru_.Get(guid))
        {
            return Future<Ref<AssetBase>>::MakeReady(std::move(cached));
        }
        if (auto it = inflight_.find(guid); it != inflight_.end())
        {
            return it->second;
        }
        inflight_.emplace(guid, ownedFuture);
    }

    auto entry = registry_.Find(guid);
    if (!entry.has_value())
    {
        HYLUX_LOG_WARN(LogAsset, "AssetSubsystem: GUID {} not in registry", guid.ToString());
        {
            std::lock_guard lock(stateMutex_);
            inflight_.erase(guid);
        }
        ownedPromise.Set(Ref<AssetBase>{});
        return ownedFuture;
    }

    Future<Ref<AssetBase>> inner = ParseAndConstruct(*entry);
    if (!inner.IsValid())
    {
        {
            std::lock_guard lock(stateMutex_);
            inflight_.erase(guid);
        }
        ownedPromise.Set(Ref<AssetBase>{});
        return ownedFuture;
    }

    inner.Then([this, guid, ownedPromise](Ref<AssetBase>& asset) mutable {
        {
            std::lock_guard lock(stateMutex_);
            if (asset)
            {
                lru_.Insert(guid, asset, asset->GetMemoryFootprint());
            }
            inflight_.erase(guid);
        }
        ownedPromise.Set(asset);
    });
    return ownedFuture;
}

Future<Ref<AssetBase>> AssetSubsystem::ParseAndConstruct(const RegistryEntry& entry)
{
    auto reader = Cooked::CookedReader::Open(*vfs_, entry.path);
    if (!reader.has_value())
    {
        return Future<Ref<AssetBase>>::MakeFailed();
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
            return MeshLoader::Load(*reader, ctx).Transform<Ref<AssetBase>>(
                [](Ref<MeshAsset>& m) -> Ref<AssetBase> { return Ref<AssetBase>(m.Get()); });
        case AssetTypeId::Material:
            return MaterialLoader::Load(*reader, ctx).Transform<Ref<AssetBase>>(
                [](Ref<MaterialAsset>& m) -> Ref<AssetBase> { return Ref<AssetBase>(m.Get()); });
        case AssetTypeId::MaterialInstance:
            return MaterialInstanceLoader::Load(*reader, ctx).Transform<Ref<AssetBase>>(
                [](Ref<MaterialInstanceAsset>& m) -> Ref<AssetBase> { return Ref<AssetBase>(m.Get()); });
        case AssetTypeId::Texture:
            return TextureLoader::Load(*reader, ctx).Transform<Ref<AssetBase>>(
                [](Ref<TextureAsset>& t) -> Ref<AssetBase> { return Ref<AssetBase>(t.Get()); });
        case AssetTypeId::Unknown:
        default:
            HYLUX_LOG_WARN(LogAsset, "AssetSubsystem: unknown type tag for '{}'", entry.path);
            return Future<Ref<AssetBase>>::MakeFailed();
    }
}

} // namespace Hylux::Asset
