/// @file
/// @brief Opaque handle value types used by RenderGraph builders and contexts.

#pragma once

#include <cstdint>

namespace Hylux::RG
{

class RGBuilder;
class RGRasterBuilder;
class RGContext;
class RenderGraph;

inline constexpr std::uint32_t kInvalidRGIndex = 0xFFFFFFFFu;

/// @brief SSA-versioned handle to a RenderGraph-managed texture. Default-constructed handles
///        are invalid; valid handles are produced by RGBuilder::CreateTexture/ImportTexture and
///        re-versioned by every RGBuilder::WriteTexture call.
class RGTextureHandle
{
public:
    constexpr RGTextureHandle() noexcept = default;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return index_ != kInvalidRGIndex; }
    [[nodiscard]] constexpr std::uint32_t Index() const noexcept { return index_; }
    [[nodiscard]] constexpr std::uint32_t Version() const noexcept { return version_; }

    [[nodiscard]] friend constexpr bool operator==(RGTextureHandle a, RGTextureHandle b) noexcept
    {
        return a.index_ == b.index_ && a.version_ == b.version_;
    }
    [[nodiscard]] friend constexpr bool operator!=(RGTextureHandle a, RGTextureHandle b) noexcept
    {
        return !(a == b);
    }

private:
    friend class RGBuilder;
    friend class RGRasterBuilder;
    friend class RGContext;
    friend class RenderGraph;

    constexpr RGTextureHandle(std::uint32_t index, std::uint32_t version) noexcept
        : index_(index), version_(version) {}

    std::uint32_t index_{kInvalidRGIndex};
    std::uint32_t version_{0};
};

/// @brief SSA-versioned handle to a RenderGraph-managed buffer. Same lifecycle rules as
///        RGTextureHandle.
class RGBufferHandle
{
public:
    constexpr RGBufferHandle() noexcept = default;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return index_ != kInvalidRGIndex; }
    [[nodiscard]] constexpr std::uint32_t Index() const noexcept { return index_; }
    [[nodiscard]] constexpr std::uint32_t Version() const noexcept { return version_; }

    [[nodiscard]] friend constexpr bool operator==(RGBufferHandle a, RGBufferHandle b) noexcept
    {
        return a.index_ == b.index_ && a.version_ == b.version_;
    }
    [[nodiscard]] friend constexpr bool operator!=(RGBufferHandle a, RGBufferHandle b) noexcept
    {
        return !(a == b);
    }

private:
    friend class RGBuilder;
    friend class RGRasterBuilder;
    friend class RGContext;
    friend class RenderGraph;

    constexpr RGBufferHandle(std::uint32_t index, std::uint32_t version) noexcept
        : index_(index), version_(version) {}

    std::uint32_t index_{kInvalidRGIndex};
    std::uint32_t version_{0};
};

} // namespace Hylux::RG
