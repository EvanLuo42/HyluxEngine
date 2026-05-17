/// @file
/// @brief Backend-neutral tagged native handle. Lets callers retrieve underlying GAPI handles
///        without leaking vulkan.hpp / d3d12.h into the RHI public surface.

#pragma once

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Tags identifying which backend handle is encoded in RHINativeHandle::value.
///        All listed handle types fit in a 64-bit slot: Vulkan dispatchable handles are
///        pointer-sized, non-dispatchable handles are uint64_t, D3D12 interfaces are
///        IUnknown pointers, console handles are pointers or 64-bit IDs.
enum class RHINativeHandleKind : std::uint32_t
{
    None = 0,

    VkInstance,
    VkPhysicalDevice,
    VkDevice,
    VkQueue,
    VkCommandPool,
    VkCommandBuffer,
    VkBuffer,
    VkImage,
    VkImageView,
    VkSampler,
    VkPipeline,
    VkPipelineCache,
    VkPipelineLayout,
    VkDescriptorPool,
    VkDescriptorSetLayout,
    VkDescriptorSet,
    VkSemaphore,
    VkFence,
    VkSwapchainKHR,
    VkSurfaceKHR,
    VkAccelerationStructureKHR,
    VkDeviceMemory,
    VmaAllocator,
    VmaAllocation,

    ID3D12Device,
    ID3D12Resource,
    ID3D12CommandAllocator,
    ID3D12CommandQueue,
    ID3D12GraphicsCommandList,
    ID3D12Fence,
    ID3D12PipelineState,
    ID3D12RootSignature,
    ID3D12DescriptorHeap,
    ID3D12QueryHeap,
    IDXGISwapChain,
    D3D12CpuDescriptorHandle,
    D3D12GpuDescriptorHandle,
    D3D12GpuVirtualAddress,

    MTLDevice,
    MTLCommandQueue,
    MTLBuffer,
    MTLTexture,
    MTLCommandBuffer,

    ConsolePS5Generic1,
    ConsolePS5Generic2,
    ConsoleXboxSeriesGeneric1,
    ConsoleXboxSeriesGeneric2,
    ConsoleSwitchNvnGeneric1,
    ConsoleSwitchNvnGeneric2,
};

/// @brief Selects which native handle to retrieve when an RHI object exposes more than one.
enum class NativeHandleQuery : std::uint32_t
{
    Primary,
    Memory,
    Allocation,
    Secondary,
};

/// @brief Opaque tagged native handle. Callers compare RHINativeHandle::kind and reinterpret
///        RHINativeHandle::value to the appropriate backend type in their own translation unit.
struct RHINativeHandle
{
    RHINativeHandleKind kind{RHINativeHandleKind::None};
    std::uint64_t       value{0};
};

static_assert(sizeof(RHINativeHandle) <= 16, "RHINativeHandle must remain a small POD");

} // namespace Hylux::RHI
