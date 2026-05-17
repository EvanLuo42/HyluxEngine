/// @file
/// @brief Sole translation unit that instantiates the Vulkan Memory Allocator implementation.
///        Compile-defs VMA_STATIC_VULKAN_FUNCTIONS=0 and VMA_DYNAMIC_VULKAN_FUNCTIONS=1 come
///        from the CMake Hylux::Vulkan INTERFACE target so VMA loads its function pointers
///        from volk's loaded dispatch tables at vmaCreateAllocator time.

#include <volk.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
