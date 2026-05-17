#pragma once

#include <cstdint>
#include <filesystem>

namespace Hylux::Renderer
{

/// @brief Aggregate configuration handed to RenderSubsystem at construction. Defaults are
///        sized for the editor PoC; production hosts override as needed.
struct RendererConfig
{
    /// @brief Number of frames the renderer overlaps with the game thread.
    std::uint32_t framesInFlight{2};

    /// @brief Capacity of the per-half transform GPU buffer.
    std::uint32_t transformBufferCapacity{1u << 20};

    /// @brief Per-frame staging ring budget.
    std::uint64_t uploadRingBytesPerFrame{16ull * 1024 * 1024};

    /// @brief Whether the PSO blob cache should be persisted across runs.
    bool enablePsoDiskCache{true};

    /// @brief Disk directory for PSO blobs. Independent of VFS by design.
    std::filesystem::path psoCacheDir{};
};

} // namespace Hylux::Renderer
