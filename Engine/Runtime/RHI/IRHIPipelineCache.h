/// @file
/// @brief Backend-side persistent pipeline cache interface. Vulkan wraps VkPipelineCache;
///        D3D12 / Metal backends may no-op the serialization round-trip until they land.
///        The engine owns one (or a small number of) IRHIPipelineCache instances and feeds
///        the pointer through GraphicsPipelineDesc::pipelineCache for every pipeline build.

#pragma once

#include "RHI/IRHIObject.h"

#include <cstddef>
#include <span>
#include <vector>

namespace Hylux::RHI
{

/// @brief Persistent backend pipeline cache. Storage is opaque; the engine treats blobs as
///        a single binary chunk it persists to disk and feeds back in on the next run.
///        Backends are free to validate the blob (driver UUID, GPU UUID, vendor) and
///        silently ignore mismatched data.
class IRHIPipelineCache : public IRHIObject
{
public:
    /// @brief Serializes the current cache contents into a backend-specific blob. Returns
    ///        an empty vector if the backend has no persistent representation or the cache
    ///        is empty.
    [[nodiscard]] virtual std::vector<std::byte> SerializeToBlob() const = 0;

    /// @brief Replaces the cache contents with the supplied blob. Silently ignored when
    ///        the blob fails the backend's validation. Returns true if the data was
    ///        accepted, false on rejection.
    virtual bool LoadFromBlob(std::span<const std::byte> blob) = 0;
};

} // namespace Hylux::RHI
