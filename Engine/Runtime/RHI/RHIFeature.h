/// @file
/// @brief Feature enumeration and FeatureSet bitmap inspired by slang-rhi. Used for
///        capability queries on IRHIAdapter and feature negotiation on IRHIDevice.

#pragma once

#include <bitset>
#include <cstdint>
#include <initializer_list>

namespace Hylux::RHI
{

/// @brief Single capability identifier. Backends report which values they support, and the
///        engine negotiates the enabled subset at device creation. Vendor SDK and capture
///        tool entries flag availability only; the typed access path goes through
///        IRHIVendorExtension / IGraphicsCaptureTool.
enum class Feature : std::uint16_t
{
    None = 0,

    HardwareDevice,
    SoftwareDevice,

    RayTracing,
    RayQuery,
    RayTracingMotionBlur,
    RayTracingOpacityMicromap,
    RayTracingDisplacementMicromap,
    MeshShader,
    TaskShader,
    Bindless,
    BindlessTextureArray,
    ConservativeRasterization,
    VariableRateShading,
    SamplerFeedback,
    WorkGraphs,
    DrawIndirectCount,
    MultiDrawIndirect,
    BufferDeviceAddress,
    DescriptorIndexing,
    DynamicRendering,
    TimelineSemaphore,
    EnhancedBarriers,
    PipelineCache,
    CooperativeVector,
    CooperativeMatrix,

    WaveOps,
    WaveSize32,
    WaveSize64,
    ShaderModel66,
    ShaderModel67,
    ShaderModel68,

    Float16,
    Float8E4M3,
    Float8E5M2,
    BFloat16,
    Int8,
    Int16,
    Int64,
    AtomicFloat,
    AtomicInt64,
    AtomicHalf,

    VendorNvApi,
    VendorAmdAgs,
    VendorIntelXeSdk,
    VendorXboxXG,
    VendorPS5Agc,
    VendorSwitchNvn,

    CaptureNsight,
    CapturePix,
    CaptureRenderDoc,
    CaptureRgp,

    Count,
};

/// @brief Compile-time upper bound for the underlying FeatureSet bitset. Grows in multiples
///        of 64; reserves headroom above Feature::Count so additions do not break ABI.
inline constexpr std::size_t kFeatureSetBits = 256;

/// @brief Fixed-size bitmap of Feature values with set/test/iterate helpers.
class FeatureSet
{
public:
    FeatureSet() = default;

    FeatureSet(std::initializer_list<Feature> features)
    {
        for (Feature f : features)
        {
            Set(f);
        }
    }

    /// @brief Marks the given feature as present.
    void Set(Feature f) noexcept
    {
        bits_.set(static_cast<std::size_t>(f));
    }

    /// @brief Removes the given feature from the set.
    void Clear(Feature f) noexcept
    {
        bits_.reset(static_cast<std::size_t>(f));
    }

    /// @brief Returns true if the feature is present.
    [[nodiscard]] bool Has(Feature f) const noexcept
    {
        return bits_.test(static_cast<std::size_t>(f));
    }

    /// @brief Returns the number of features currently set.
    [[nodiscard]] std::size_t Count() const noexcept
    {
        return bits_.count();
    }

    /// @brief Returns true if no features are set.
    [[nodiscard]] bool Empty() const noexcept
    {
        return bits_.none();
    }

    [[nodiscard]] FeatureSet operator|(const FeatureSet& other) const noexcept
    {
        FeatureSet result;
        result.bits_ = bits_ | other.bits_;
        return result;
    }

    [[nodiscard]] FeatureSet operator&(const FeatureSet& other) const noexcept
    {
        FeatureSet result;
        result.bits_ = bits_ & other.bits_;
        return result;
    }

    FeatureSet& operator|=(const FeatureSet& other) noexcept
    {
        bits_ |= other.bits_;
        return *this;
    }

    FeatureSet& operator&=(const FeatureSet& other) noexcept
    {
        bits_ &= other.bits_;
        return *this;
    }

    [[nodiscard]] bool operator==(const FeatureSet& other) const noexcept
    {
        return bits_ == other.bits_;
    }

private:
    std::bitset<kFeatureSetBits> bits_{};
};

static_assert(static_cast<std::size_t>(Feature::Count) <= kFeatureSetBits,
              "Feature::Count must fit within the FeatureSet bitset width");

} // namespace Hylux::RHI
