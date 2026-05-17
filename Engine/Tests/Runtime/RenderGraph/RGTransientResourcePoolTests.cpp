/// @file
/// @brief Unit tests for RGTransientResourcePool. Uses minimal FakeTexture / FakeBuffer
///        implementations of IRHITexture / IRHIBuffer so the pool's logic can be
///        exercised without a live IRHIDevice. Factories are injected lambdas that count
///        creations, letting us assert reuse vs create directly.

#include "Core/Memory/MakeRef.h"
#include "Core/Memory/Ref.h"
#include "RHI/IRHIBuffer.h"
#include "RHI/IRHITexture.h"
#include "RenderGraph/Internal/RGTransientResourcePool.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <string>
#include <string_view>

TEST_SUITE_BEGIN("RenderGraph");

using namespace Hylux;
using namespace Hylux::RG::Internal;

namespace
{

class FakeTexture final : public RHI::IRHITexture
{
public:
    explicit FakeTexture(const RHI::TextureDesc& desc) noexcept : desc_(desc) {}

    [[nodiscard]] const RHI::TextureDesc& GetDesc() const noexcept override { return desc_; }
    [[nodiscard]] RHI::BindlessIndex      GetBindlessIndex(RHI::BindlessKind) const noexcept override
    {
        return RHI::BindlessIndex::Invalid;
    }
    [[nodiscard]] RHI::IRHIDevice*  GetDevice() const noexcept override { return nullptr; }
    [[nodiscard]] RHI::RHINativeHandle GetNativeHandle(RHI::NativeHandleQuery) const noexcept override
    {
        return RHI::RHINativeHandle{};
    }
    void                       SetDebugName(std::string_view name) override { debugName_ = std::string(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return debugName_; }

private:
    RHI::TextureDesc desc_;
    std::string      debugName_;
};

class FakeBuffer final : public RHI::IRHIBuffer
{
public:
    explicit FakeBuffer(const RHI::BufferDesc& desc) noexcept : desc_(desc) {}

    [[nodiscard]] const RHI::BufferDesc& GetDesc() const noexcept override { return desc_; }
    [[nodiscard]] std::uint64_t GetSize() const noexcept override { return desc_.size; }
    [[nodiscard]] std::uint64_t GetGpuAddress() const noexcept override { return 0; }
    void*                       Map(std::uint64_t, std::uint64_t) override { return nullptr; }
    void                        Unmap() override {}
    [[nodiscard]] RHI::BindlessIndex GetBindlessIndex(RHI::BindlessKind) const noexcept override
    {
        return RHI::BindlessIndex::Invalid;
    }
    [[nodiscard]] RHI::IRHIDevice*    GetDevice() const noexcept override { return nullptr; }
    [[nodiscard]] RHI::RHINativeHandle GetNativeHandle(RHI::NativeHandleQuery) const noexcept override
    {
        return RHI::RHINativeHandle{};
    }
    void                       SetDebugName(std::string_view name) override { debugName_ = std::string(name); }
    [[nodiscard]] std::string_view GetDebugName() const noexcept override { return debugName_; }

private:
    RHI::BufferDesc desc_;
    std::string     debugName_;
};

[[nodiscard]] RHI::TextureDesc MakeTextureDesc(std::uint32_t w, std::uint32_t h,
                                               RHI::Format format = RHI::Format::RGBA8Unorm)
{
    RHI::TextureDesc d{};
    d.dimension     = RHI::TextureDimension::Tex2D;
    d.format        = format;
    d.extent.width  = w;
    d.extent.height = h;
    d.extent.depth  = 1;
    d.mipLevels     = 1;
    d.arrayLayers   = 1;
    d.sampleCount   = 1;
    d.usage         = RHI::TextureUsage::ColorAttachment;
    d.memory        = RHI::MemoryUsage::GpuOnly;
    return d;
}

[[nodiscard]] RHI::BufferDesc MakeBufferDesc(std::uint64_t size,
                                             RHI::BufferUsage usage = RHI::BufferUsage::StorageBuffer)
{
    RHI::BufferDesc d{};
    d.size   = size;
    d.usage  = usage;
    d.memory = RHI::MemoryUsage::GpuOnly;
    return d;
}

struct CountingFactories
{
    int textureCreates{0};
    int bufferCreates{0};

    [[nodiscard]] RGTransientResourcePool::TextureFactory MakeTextureFactory()
    {
        return [this](const RHI::TextureDesc& desc) -> Ref<RHI::IRHITexture> {
            ++textureCreates;
            return MakeRef<FakeTexture>(desc);
        };
    }

    [[nodiscard]] RGTransientResourcePool::BufferFactory MakeBufferFactory()
    {
        return [this](const RHI::BufferDesc& desc) -> Ref<RHI::IRHIBuffer> {
            ++bufferCreates;
            return MakeRef<FakeBuffer>(desc);
        };
    }
};

} // namespace

TEST_CASE("RGTransientResourcePool: first Acquire creates; subsequent matching Acquire reuses")
{
    CountingFactories       factories;
    RGTransientResourcePool pool(factories.MakeTextureFactory(), factories.MakeBufferFactory(),
                                 /*framesInFlight=*/2);

    const RHI::TextureDesc desc = MakeTextureDesc(1280, 720);

    Ref<RHI::IRHITexture> first = pool.AcquireTexture(desc, 1);
    REQUIRE(first);
    CHECK(factories.textureCreates == 1);
    CHECK(pool.GetStats().textureEntries == 1u);

    // Frame 1 + framesInFlight 2 -> first reusable at frame 3 onward.
    Ref<RHI::IRHITexture> reused = pool.AcquireTexture(desc, 3);
    REQUIRE(reused);
    CHECK(reused.Get() == first.Get());
    CHECK(factories.textureCreates == 1);
    CHECK(pool.GetStats().textureReuseCount == 1u);
}

TEST_CASE("RGTransientResourcePool: same-frame second Acquire allocates a distinct resource")
{
    CountingFactories       factories;
    RGTransientResourcePool pool(factories.MakeTextureFactory(), factories.MakeBufferFactory(), 2);

    const RHI::TextureDesc desc = MakeTextureDesc(512, 512);

    Ref<RHI::IRHITexture> a = pool.AcquireTexture(desc, 7);
    Ref<RHI::IRHITexture> b = pool.AcquireTexture(desc, 7);
    REQUIRE(a);
    REQUIRE(b);
    CHECK(a.Get() != b.Get());
    CHECK(factories.textureCreates == 2);
    CHECK(pool.GetStats().textureEntries == 2u);
}

TEST_CASE("RGTransientResourcePool: in-flight Acquire skips entries not yet GPU-idle")
{
    CountingFactories       factories;
    RGTransientResourcePool pool(factories.MakeTextureFactory(), factories.MakeBufferFactory(),
                                 /*framesInFlight=*/3);

    const RHI::TextureDesc desc = MakeTextureDesc(256, 256);

    Ref<RHI::IRHITexture> f1 = pool.AcquireTexture(desc, 10);
    REQUIRE(f1);

    // framesInFlight=3 so the entry borrowed at frame 10 is reusable at frame 13 onward.
    Ref<RHI::IRHITexture> f11 = pool.AcquireTexture(desc, 11);
    Ref<RHI::IRHITexture> f12 = pool.AcquireTexture(desc, 12);
    CHECK(f11.Get() != f1.Get());
    CHECK(f12.Get() != f1.Get());
    CHECK(f12.Get() != f11.Get());
    CHECK(factories.textureCreates == 3);

    Ref<RHI::IRHITexture> f13 = pool.AcquireTexture(desc, 13);
    // f1 was at frame 10 (oldest), reusable now.
    CHECK(f13.Get() == f1.Get());
    CHECK(factories.textureCreates == 3);
}

TEST_CASE("RGTransientResourcePool: differing descs do not share entries")
{
    CountingFactories       factories;
    RGTransientResourcePool pool(factories.MakeTextureFactory(), factories.MakeBufferFactory(), 1);

    const RHI::TextureDesc small = MakeTextureDesc(64, 64);
    const RHI::TextureDesc large = MakeTextureDesc(1024, 1024);

    Ref<RHI::IRHITexture> a = pool.AcquireTexture(small, 1);
    Ref<RHI::IRHITexture> b = pool.AcquireTexture(large, 1);
    REQUIRE(a);
    REQUIRE(b);
    CHECK(a.Get() != b.Get());

    // Reuse small at frame 3 — should reuse a, not b.
    Ref<RHI::IRHITexture> a2 = pool.AcquireTexture(small, 3);
    CHECK(a2.Get() == a.Get());
    CHECK(factories.textureCreates == 2);
}

TEST_CASE("RGTransientResourcePool: EndFrame drops entries idle beyond the window")
{
    CountingFactories       factories;
    RGTransientResourcePool pool(factories.MakeTextureFactory(), factories.MakeBufferFactory(), 1);

    const RHI::TextureDesc desc = MakeTextureDesc(128, 128);

    {
        Ref<RHI::IRHITexture> ref = pool.AcquireTexture(desc, 5);
        REQUIRE(ref);
    }
    CHECK(pool.GetStats().textureEntries == 1u);

    // Idle window 4 from frame 9 -> cutoff = 5; entry's lastUsedFrame (5) >= cutoff -> retained.
    pool.EndFrame(9, 4);
    CHECK(pool.GetStats().textureEntries == 1u);

    // Idle window 3 from frame 9 -> cutoff = 6; entry < cutoff -> evicted.
    pool.EndFrame(9, 3);
    CHECK(pool.GetStats().textureEntries == 0u);
}

TEST_CASE("RGTransientResourcePool: factory returning null leaves the pool untouched")
{
    int                     callCount = 0;
    RGTransientResourcePool pool(
        [&callCount](const RHI::TextureDesc&) -> Ref<RHI::IRHITexture> {
            ++callCount;
            return Ref<RHI::IRHITexture>{};
        },
        [](const RHI::BufferDesc&) -> Ref<RHI::IRHIBuffer> { return Ref<RHI::IRHIBuffer>{}; },
        1);

    const RHI::TextureDesc desc = MakeTextureDesc(64, 64);
    Ref<RHI::IRHITexture>  result = pool.AcquireTexture(desc, 1);
    CHECK_FALSE(result);
    CHECK(callCount == 1);
    CHECK(pool.GetStats().textureEntries == 0u);
    CHECK(pool.GetStats().textureCreateCount == 0u);
}

TEST_CASE("RGTransientResourcePool: buffer acquire mirrors texture semantics")
{
    CountingFactories       factories;
    RGTransientResourcePool pool(factories.MakeTextureFactory(), factories.MakeBufferFactory(), 2);

    const RHI::BufferDesc desc = MakeBufferDesc(4096);

    Ref<RHI::IRHIBuffer> first = pool.AcquireBuffer(desc, 1);
    REQUIRE(first);
    CHECK(factories.bufferCreates == 1);

    Ref<RHI::IRHIBuffer> sameFrame = pool.AcquireBuffer(desc, 1);
    REQUIRE(sameFrame);
    CHECK(sameFrame.Get() != first.Get());
    CHECK(factories.bufferCreates == 2);

    Ref<RHI::IRHIBuffer> reused = pool.AcquireBuffer(desc, 3);
    REQUIRE(reused);
    CHECK(reused.Get() == first.Get());
    CHECK(factories.bufferCreates == 2);
}

TEST_CASE("RGTransientResourcePool: Reset drops every entry")
{
    CountingFactories       factories;
    RGTransientResourcePool pool(factories.MakeTextureFactory(), factories.MakeBufferFactory(), 1);

    pool.AcquireTexture(MakeTextureDesc(32, 32), 1);
    pool.AcquireBuffer(MakeBufferDesc(64), 1);
    CHECK(pool.GetStats().textureEntries == 1u);
    CHECK(pool.GetStats().bufferEntries == 1u);

    pool.Reset();
    CHECK(pool.GetStats().textureEntries == 0u);
    CHECK(pool.GetStats().bufferEntries == 0u);
}

TEST_SUITE_END();
