/// @file
/// @brief Ray tracing acceleration structure interface and build descriptors.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIForward.h"

#include <cstdint>
#include <span>

namespace Hylux::RHI
{

/// @brief Geometry kind embedded within a BlasDesc geometry entry.
enum class GeometryKind : std::uint32_t
{
    Triangles = 0,
    AxisAlignedBoundingBoxes,
};

/// @brief Per-entry geometry record consumed by BlasDesc.
struct GeometryDesc
{
    GeometryKind  kind{GeometryKind::Triangles};
    IRHIBuffer*   vertexBuffer{nullptr};
    std::uint64_t vertexOffset{0};
    std::uint32_t vertexStride{0};
    std::uint32_t vertexCount{0};
    IRHIBuffer*   indexBuffer{nullptr};
    std::uint64_t indexOffset{0};
    std::uint32_t indexCount{0};
    IndexType     indexType{IndexType::Uint32};
    IRHIBuffer*   transformBuffer{nullptr};
    std::uint64_t transformOffset{0};
    IRHIBuffer*   aabbBuffer{nullptr};
    std::uint64_t aabbOffset{0};
    std::uint32_t aabbCount{0};
    std::uint32_t aabbStride{0};
    bool          opaque{true};
};

/// @brief Bottom-level acceleration structure description.
struct BlasDesc
{
    std::span<const GeometryDesc> geometries{};
    bool                          allowUpdate{false};
    bool                          allowCompaction{false};
    bool                          preferFastTrace{true};
    bool                          preferFastBuild{false};
};

/// @brief Top-level acceleration structure description. Instance buffer is consumed by the
///        build call; this descriptor only sets sizing hints.
struct TlasDesc
{
    std::uint32_t maxInstanceCount{0};
    bool          allowUpdate{false};
    bool          allowCompaction{false};
    bool          preferFastTrace{true};
    bool          preferFastBuild{false};
};

/// @brief Aggregate build description supplied to IRHICommandList::BuildAccelerationStructures.
struct AccelerationStructureBuildDesc
{
    IRHIAccelerationStructure* target{nullptr};
    IRHIAccelerationStructure* source{nullptr};
    IRHIBuffer*                scratchBuffer{nullptr};
    std::uint64_t              scratchOffset{0};
    IRHIBuffer*                instanceBuffer{nullptr};
    std::uint64_t              instanceOffset{0};
    std::uint32_t              instanceCount{0};
    bool                       update{false};
};

/// @brief Reference-counted handle to a built BLAS or TLAS.
class IRHIAccelerationStructure : public IRHIObject
{
public:
    /// @brief Returns the GPU virtual address of the structure, used to populate TLAS instances.
    [[nodiscard]] virtual std::uint64_t GetGpuAddress() const noexcept = 0;

    /// @brief Returns the size in bytes of the structure storage buffer.
    [[nodiscard]] virtual std::uint64_t GetSize() const noexcept = 0;

    /// @brief Returns the bindless table index, or BindlessIndex::Invalid if not registered.
    [[nodiscard]] virtual BindlessIndex GetBindlessIndex() const noexcept = 0;
};

} // namespace Hylux::RHI
