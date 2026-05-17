/// @file
/// @brief Describes how DrawListBuilder packs per-instance data into the instance buffer.
///        Stage 4a establishes the schema; per-frame upload through UploadHeapManager
///        lands with the indirect-draw work.

#pragma once

#include <cstdint>

namespace Hylux::Renderer
{

/// @brief Per-instance data packing. Offsets are byte offsets into the per-instance
///        record; 0xFFFFFFFF marks "not present". The custom payload sits at the tail.
struct InstanceDataLayout
{
    std::uint32_t stride{0};
    std::uint32_t worldMatrixOffset{0xFFFFFFFFu};
    std::uint32_t materialIndexOffset{0xFFFFFFFFu};
    std::uint32_t customDataSize{0};
    bool          uploadPerFrame{true};
};

} // namespace Hylux::Renderer
