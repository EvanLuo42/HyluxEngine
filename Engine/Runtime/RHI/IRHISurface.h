/// @file
/// @brief Window surface and swapchain interfaces. Window creation is out of scope;
///        a PlatformWindowHandle is supplied by the editor / launcher windowing subsystem.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIFormat.h"
#include "RHI/RHIForward.h"
#include "RHI/RHITypes.h"

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Tag identifying the windowing system that produced a PlatformWindowHandle.
enum class PlatformWindowSystem : std::uint32_t
{
    None = 0,
    Win32,
    Xlib,
    Xcb,
    Wayland,
    AppKit,
    Android,
    PS5,
    XboxCore,
    Switch,
};

/// @brief Backend-neutral native window handle bundle. Concrete fields are reinterpreted
///        per platform: on Win32, nativeWindow is HWND and nativeDisplay is HINSTANCE;
///        on Xlib, nativeWindow is a Window and nativeDisplay is a Display*; etc.
struct PlatformWindowHandle
{
    PlatformWindowSystem system{PlatformWindowSystem::None};
    void*                nativeWindow{nullptr};
    void*                nativeDisplay{nullptr};
};

/// @brief Swapchain creation description.
struct SwapchainDesc
{
    Format         format{Format::BGRA8UnormSrgb};
    std::uint32_t  bufferCount{3};
    Extent2D       extent{};
    PresentMode    presentMode{PresentMode::Fifo};
    SwapchainUsage usage{SwapchainUsage::ColorAttachment};
    bool           srgbView{true};
};

/// @brief Reference-counted handle wrapping a backend surface created over a window handle.
class IRHISurface : public IRHIObject
{
public:
    /// @brief Returns the window handle the surface was created over.
    [[nodiscard]] virtual const PlatformWindowHandle& GetWindowHandle() const noexcept = 0;

    /// @brief Returns the current native size of the window. Used to drive swapchain resize.
    [[nodiscard]] virtual Extent2D GetCurrentExtent() const noexcept = 0;
};

/// @brief Reference-counted handle to a swapchain attached to an IRHISurface.
class IRHISwapchain : public IRHIObject
{
public:
    /// @brief Returns the descriptor the swapchain was created with.
    [[nodiscard]] virtual const SwapchainDesc& GetDesc() const noexcept = 0;

    /// @brief Returns the parent surface.
    [[nodiscard]] virtual IRHISurface* GetSurface() const noexcept = 0;

    /// @brief Returns the swapchain image at the given index as a texture, or nullptr if
    ///        the index is out of range.
    [[nodiscard]] virtual IRHITexture* GetBackBuffer(std::uint32_t index) const noexcept = 0;

    /// @brief Returns the configured image count.
    [[nodiscard]] virtual std::uint32_t GetBackBufferCount() const noexcept = 0;

    /// @brief Acquires the next image. The returned index is the value to pass to
    ///        IRHIQueue::Present. If outSignalFence is non-null, the fence is signaled to
    ///        the listed value once the image is available for rendering.
    virtual bool AcquireNextImage(IRHIFence* outSignalFence, std::uint64_t signalValue,
                                  std::uint32_t& outImageIndex) = 0;

    /// @brief Re-creates the swapchain to match the new extent. Existing back-buffer
    ///        IRHITexture handles are invalidated.
    virtual bool Resize(Extent2D newExtent) = 0;
};

} // namespace Hylux::RHI
