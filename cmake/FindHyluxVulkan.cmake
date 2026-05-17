# =============================================================================
# FindHyluxVulkan.cmake
#
# Bundles the Vulkan stack the HyluxEngine RHI consumes into one INTERFACE
# target Hylux::Vulkan.
#
# Sources:
#   - volk                       (vcpkg)  -- dynamic loader; no link to vulkan-1
#   - vulkan-headers             (vcpkg)  -- pulled transitively by volk
#   - vulkan-memory-allocator    (vcpkg)  -- header-only VMA
#
# Defines:
#   Hylux::Vulkan  INTERFACE target with includes, link deps, and the standard
#                  compile-defs every Vulkan TU needs (VK_NO_PROTOTYPES,
#                  VMA_*_VULKAN_FUNCTIONS, VK_USE_PLATFORM_*).
# =============================================================================

if (TARGET Hylux::Vulkan)
    return()
endif ()

find_package(volk                   CONFIG REQUIRED)
find_package(VulkanMemoryAllocator  CONFIG REQUIRED)

add_library(Hylux_Vulkan INTERFACE)
add_library(Hylux::Vulkan ALIAS Hylux_Vulkan)

target_link_libraries(Hylux_Vulkan INTERFACE
    volk::volk
    GPUOpen::VulkanMemoryAllocator)

target_compile_definitions(Hylux_Vulkan INTERFACE
    VK_NO_PROTOTYPES=1
    VMA_STATIC_VULKAN_FUNCTIONS=0
    VMA_DYNAMIC_VULKAN_FUNCTIONS=1)

if (WIN32)
    target_compile_definitions(Hylux_Vulkan INTERFACE VK_USE_PLATFORM_WIN32_KHR=1)
elseif (HYLUX_PLATFORM_LINUX)
    target_compile_definitions(Hylux_Vulkan INTERFACE VK_USE_PLATFORM_XCB_KHR=1)
endif ()

message(STATUS "Hylux::Vulkan = volk + VulkanMemoryAllocator (no LunarG dependency)")
