/// @file
/// @brief Forward declarations for all RHI public interfaces and descriptor structs.

#pragma once

#include <cstdint>

namespace Hylux::RHI
{

class IRHIObject;
class IRHIInstance;
class IRHIAdapter;
class IRHIDevice;
class IRHIQueue;
class IRHIFence;
class IRHISurface;
class IRHISwapchain;
class IRHIBuffer;
class IRHITexture;
class IRHITextureView;
class IRHISampler;
class IRHIShaderModule;
class IRHIPipelineLayout;
class IRHIGraphicsPipeline;
class IRHIComputePipeline;
class IRHIRayTracingPipeline;
class IRHIPipelineCache;
class IRHIAccelerationStructure;
class IRHIBindlessHeap;
class IRHIDescriptorSetLayout;
class IRHIDescriptorSet;
class IRHICommandPool;
class IRHICommandList;
class IRHIQueryPool;
class IRHIVendorExtension;
class IGraphicsCaptureTool;
class ICaptureScope;
class IGpuRangeProfiler;
class IGpuMetricSet;
class IGpuTraceCapture;
class IShaderProfiler;
class IGpuCrashReporter;

struct InstanceDesc;
struct AdapterDesc;
struct DeviceDesc;
struct DeviceLimits;
struct BindlessHeapSizes;
struct QueueDesc;
struct SubmitDesc;
struct PresentDesc;
struct FenceWaitDesc;
struct FenceSignalDesc;
struct SwapchainDesc;
struct PlatformWindowHandle;
struct BufferDesc;
struct TextureDesc;
struct TextureViewDesc;
struct SamplerDesc;
struct ShaderBytecode;
struct PipelineLayoutDesc;
struct GraphicsPipelineDesc;
struct ComputePipelineDesc;
struct RayTracingPipelineDesc;
struct VertexInputDesc;
struct RasterizerDesc;
struct DepthStencilDesc;
struct BlendDesc;
struct RenderingInfo;
struct RenderingAttachment;
struct BlasDesc;
struct TlasDesc;
struct GeometryDesc;
struct DescriptorSetLayoutDesc;
struct DispatchRaysDesc;
struct AccelerationStructureBuildDesc;
struct BufferBarrier;
struct TextureBarrier;
struct MemoryBarrier;
struct BarrierGroup;
struct VertexBufferBinding;
struct TextureRegion;
struct BufferTextureLayout;
struct ResolveDesc;
struct ClearColorValue;
struct CaptureMarkerColor;
struct CaptureRequest;
struct GpuMetricDesc;
struct GpuMetricValue;
struct GpuTraceDesc;
struct ShaderSampleStats;
struct ShaderBinaryRegistration;
struct ResourceRegistration;
struct GpuFaultInfo;

struct RHINativeHandle;
struct FormatSupport;

class FeatureSet;
enum class BindlessIndex : std::uint32_t;

enum class Feature : std::uint16_t;
enum class Format : std::uint32_t;
enum class DeviceType : std::uint32_t;
enum class QueueType : std::uint32_t;
enum class ShaderStage : std::uint32_t;
enum class ShaderBytecodeFormat : std::uint32_t;
enum class MemoryUsage : std::uint32_t;
enum class BindlessKind : std::uint32_t;
enum class CaptureToolKind : std::uint32_t;
enum class CaptureActivity : std::uint32_t;
enum class CaptureStatus : std::uint32_t;
enum class GpuCrashReporterKind : std::uint32_t;
enum class GpuCrashStatus : std::uint32_t;
enum class GpuFaultKind : std::uint32_t;
enum class VendorExtensionKind : std::uint32_t;
enum class RhiValidationLevel : std::uint32_t;
enum class GapiValidationLevel : std::uint32_t;
enum class IndexType : std::uint32_t;
enum class PrimitiveTopology : std::uint32_t;
enum class QueryType : std::uint32_t;
enum class PresentMode : std::uint32_t;
enum class TextureDimension : std::uint32_t;
enum class FilterMode : std::uint32_t;
enum class MipFilterMode : std::uint32_t;
enum class AddressMode : std::uint32_t;
enum class CompareOp : std::uint32_t;
enum class BlendFactor : std::uint32_t;
enum class BlendOp : std::uint32_t;
enum class CullMode : std::uint32_t;
enum class FrontFace : std::uint32_t;
enum class FillMode : std::uint32_t;
enum class StencilOp : std::uint32_t;
enum class LoadOp : std::uint32_t;
enum class StoreOp : std::uint32_t;
enum class ImageLayout : std::uint32_t;
enum class NativeHandleQuery : std::uint32_t;

} // namespace Hylux::RHI
