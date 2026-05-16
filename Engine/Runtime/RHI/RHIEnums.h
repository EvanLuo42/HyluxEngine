/// @file
/// @brief Common enum types used across the RHI public surface.

#pragma once

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Identifies which underlying graphics API a device is backed by.
enum class DeviceType : std::uint32_t
{
    Null = 0,
    Vulkan,
    D3D12,
    Metal,
    PS5,
    XboxSeries,
    Switch,
};

/// @brief Physical queue family the engine recognizes. Backends map to native queues.
enum class QueueType : std::uint32_t
{
    Graphics = 0,
    Compute,
    Copy,
    VideoEncode,
    VideoDecode,
    Count,
};

/// @brief Shader stages addressable as bit flags within a pipeline layout or barrier.
enum class ShaderStage : std::uint32_t
{
    None            = 0,
    Vertex          = 1u << 0,
    Hull            = 1u << 1,
    Domain          = 1u << 2,
    Geometry        = 1u << 3,
    Pixel           = 1u << 4,
    Compute         = 1u << 5,
    Task            = 1u << 6,
    Mesh            = 1u << 7,
    RayGeneration   = 1u << 8,
    AnyHit          = 1u << 9,
    ClosestHit      = 1u << 10,
    Miss            = 1u << 11,
    Intersection    = 1u << 12,
    Callable        = 1u << 13,
    AllGraphics     = Vertex | Hull | Domain | Geometry | Pixel | Task | Mesh,
    AllRayTracing   = RayGeneration | AnyHit | ClosestHit | Miss | Intersection | Callable,
    All             = 0xFFFFFFFFu,
};

[[nodiscard]] constexpr ShaderStage operator|(ShaderStage a, ShaderStage b) noexcept
{
    return static_cast<ShaderStage>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr ShaderStage operator&(ShaderStage a, ShaderStage b) noexcept
{
    return static_cast<ShaderStage>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

constexpr ShaderStage& operator|=(ShaderStage& a, ShaderStage b) noexcept { a = a | b; return a; }
constexpr ShaderStage& operator&=(ShaderStage& a, ShaderStage b) noexcept { a = a & b; return a; }

/// @brief Identifies the encoded form of shader bytecode passed to CreateShaderModule.
enum class ShaderBytecodeFormat : std::uint32_t
{
    Spirv = 0,
    Dxil,
    Dxbc,
    Msl,
    MslAir,
    Pssl,
    Nvn,
    NvnGlsl,
    Agc,
    Wgsl,
};

/// @brief Memory placement hint that drives backend allocator and heap selection.
enum class MemoryUsage : std::uint32_t
{
    GpuOnly = 0,
    CpuOnly,
    CpuToGpu,
    GpuToCpu,
    CpuGpuShared,
};

/// @brief Which bindless heap a descriptor binds to. D3D12 hardware exposes exactly two
///        shader-visible heap kinds; Vulkan supports both via descriptor indexing.
enum class BindlessKind : std::uint32_t
{
    SrvCbvUav = 0,
    Sampler,
    Count,
};

/// @brief Supported graphics capture / GPU profiler integration kinds.
enum class CaptureToolKind : std::uint32_t
{
    Auto = 0,
    None,
    Nsight,
    Pix,
    RenderDoc,
    Rgp,
};

/// @brief Activity kind for IGraphicsCaptureTool::RequestCapture. Maps to Nsight Graphics
///        activities, PIX capture modes, RenderDoc capture variants, and RGP captures.
enum class CaptureActivity : std::uint32_t
{
    FrameCapture = 0,
    GpuTrace,
    GenerateCppCapture,
    RangeProfiler,
    ShaderProfiler,
};

/// @brief Lifecycle state of a submitted capture request.
enum class CaptureStatus : std::uint32_t
{
    Idle = 0,
    Pending,
    InProgress,
    Completed,
    Failed,
};

/// @brief Supported GPU crash / post-mortem dump backend kinds for IGpuCrashReporter.
enum class GpuCrashReporterKind : std::uint32_t
{
    Auto = 0,
    None,
    NvAftermath,
    D3D12Dred,
    AmdGpuCrashAnalyzer,
};

/// @brief Lifecycle state returned by IGpuCrashReporter::PollCrashStatus.
enum class GpuCrashStatus : std::uint32_t
{
    NotCrashed = 0,
    Collecting,
    DumpReady,
    Timeout,
    Unavailable,
};

/// @brief Classification of a detected GPU fault, surfaced via GpuFaultInfo::kind.
enum class GpuFaultKind : std::uint32_t
{
    Unknown = 0,
    Hang,
    PageFault,
    InvalidParameter,
    OutOfMemory,
    DeviceReset,
    DriverError,
};

/// @brief Vendor-provided SDK adapters surfaced through IRHIVendorExtension.
enum class VendorExtensionKind : std::uint32_t
{
    None = 0,
    NvApi,
    AmdAgs,
    IntelXeSdk,
    XboxXG,
    PS5Agc,
    SwitchNvn,
};

/// @brief Index buffer element width.
enum class IndexType : std::uint32_t
{
    Uint16 = 0,
    Uint32,
};

/// @brief Primitive assembly mode for graphics draws.
enum class PrimitiveTopology : std::uint32_t
{
    PointList = 0,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    PatchList,
};

/// @brief Query object kind used by IRHIQueryPool.
enum class QueryType : std::uint32_t
{
    Timestamp = 0,
    Occlusion,
    OcclusionPrecise,
    PipelineStatistics,
    AccelerationStructureCompactedSize,
    AccelerationStructureSerializationSize,
};

/// @brief Swapchain present mode.
enum class PresentMode : std::uint32_t
{
    Immediate = 0,
    Fifo,
    FifoRelaxed,
    Mailbox,
};

/// @brief Texture dimensionality.
enum class TextureDimension : std::uint32_t
{
    Tex1D = 0,
    Tex2D,
    Tex3D,
    TexCube,
    Tex1DArray,
    Tex2DArray,
    TexCubeArray,
};

/// @brief Sampler magnification / minification filter.
enum class FilterMode : std::uint32_t
{
    Nearest = 0,
    Linear,
};

/// @brief Sampler mip filter mode.
enum class MipFilterMode : std::uint32_t
{
    Nearest = 0,
    Linear,
};

/// @brief Sampler address mode applied per UVW axis.
enum class AddressMode : std::uint32_t
{
    Repeat = 0,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
    MirrorClampToEdge,
};

/// @brief Comparison operator used by samplers, depth, and stencil state.
enum class CompareOp : std::uint32_t
{
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

/// @brief Blend factor applied to source or destination color/alpha.
enum class BlendFactor : std::uint32_t
{
    Zero = 0,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate,
    Src1Color,
    OneMinusSrc1Color,
    Src1Alpha,
    OneMinusSrc1Alpha,
};

/// @brief Per-attachment blend equation.
enum class BlendOp : std::uint32_t
{
    Add = 0,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

/// @brief Rasterizer cull mode.
enum class CullMode : std::uint32_t
{
    None = 0,
    Front,
    Back,
};

/// @brief Triangle winding interpretation.
enum class FrontFace : std::uint32_t
{
    CounterClockwise = 0,
    Clockwise,
};

/// @brief Polygon fill mode.
enum class FillMode : std::uint32_t
{
    Solid = 0,
    Wireframe,
};

/// @brief Stencil operation applied on pass/fail.
enum class StencilOp : std::uint32_t
{
    Keep = 0,
    Zero,
    Replace,
    IncrementClamp,
    DecrementClamp,
    Invert,
    IncrementWrap,
    DecrementWrap,
};

/// @brief Attachment load behavior at the start of a rendering pass.
enum class LoadOp : std::uint32_t
{
    Load = 0,
    Clear,
    DontCare,
};

/// @brief Attachment store behavior at the end of a rendering pass.
enum class StoreOp : std::uint32_t
{
    Store = 0,
    DontCare,
    Resolve,
};

/// @brief Image layout values matching Vulkan sync2 / D3D12 enhanced barriers.
enum class ImageLayout : std::uint32_t
{
    Undefined = 0,
    General,
    ColorAttachment,
    DepthStencilAttachment,
    DepthStencilReadOnly,
    ShaderReadOnly,
    TransferSrc,
    TransferDst,
    Present,
    RayTracingShared,
};

/// @brief BufferUsage bit flags. Combinable via bitwise operators on the underlying type.
enum class BufferUsage : std::uint32_t
{
    None            = 0,
    TransferSrc     = 1u << 0,
    TransferDst     = 1u << 1,
    UniformBuffer   = 1u << 2,
    StorageBuffer   = 1u << 3,
    IndexBuffer     = 1u << 4,
    VertexBuffer    = 1u << 5,
    IndirectBuffer  = 1u << 6,
    ShaderDeviceAddress = 1u << 7,
    AccelerationStructureBuildInput = 1u << 8,
    AccelerationStructureStorage    = 1u << 9,
    ShaderBindingTable              = 1u << 10,
};

[[nodiscard]] constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) noexcept
{
    return static_cast<BufferUsage>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr BufferUsage operator&(BufferUsage a, BufferUsage b) noexcept
{
    return static_cast<BufferUsage>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

constexpr BufferUsage& operator|=(BufferUsage& a, BufferUsage b) noexcept { a = a | b; return a; }

/// @brief TextureUsage bit flags. Combinable via bitwise operators.
enum class TextureUsage : std::uint32_t
{
    None             = 0,
    TransferSrc      = 1u << 0,
    TransferDst      = 1u << 1,
    SampledImage     = 1u << 2,
    StorageImage     = 1u << 3,
    ColorAttachment  = 1u << 4,
    DepthStencilAttachment = 1u << 5,
    InputAttachment        = 1u << 6,
    TransientAttachment    = 1u << 7,
};

[[nodiscard]] constexpr TextureUsage operator|(TextureUsage a, TextureUsage b) noexcept
{
    return static_cast<TextureUsage>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr TextureUsage operator&(TextureUsage a, TextureUsage b) noexcept
{
    return static_cast<TextureUsage>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

constexpr TextureUsage& operator|=(TextureUsage& a, TextureUsage b) noexcept { a = a | b; return a; }

/// @brief Swapchain usage bit flags.
enum class SwapchainUsage : std::uint32_t
{
    None              = 0,
    ColorAttachment   = 1u << 0,
    TransferDst       = 1u << 1,
    StorageImage      = 1u << 2,
};

[[nodiscard]] constexpr SwapchainUsage operator|(SwapchainUsage a, SwapchainUsage b) noexcept
{
    return static_cast<SwapchainUsage>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

/// @brief Adapter capability flags reported by IRHIAdapter::GetDesc.
enum class AdapterFlags : std::uint32_t
{
    None       = 0,
    Discrete   = 1u << 0,
    Integrated = 1u << 1,
    Virtual    = 1u << 2,
    Software   = 1u << 3,
};

/// @brief Command pool capability flags.
enum class CommandPoolFlagBits : std::uint32_t
{
    None              = 0,
    Transient         = 1u << 0,
    AllowIndividualReset = 1u << 1,
};

[[nodiscard]] constexpr CommandPoolFlagBits operator|(CommandPoolFlagBits a, CommandPoolFlagBits b) noexcept
{
    return static_cast<CommandPoolFlagBits>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

/// @brief Wrapper alias for CommandPoolFlagBits used in descriptor structs.
using CommandPoolFlags = CommandPoolFlagBits;

} // namespace Hylux::RHI
