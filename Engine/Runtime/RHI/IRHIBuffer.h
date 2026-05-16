/// @file
/// @brief GPU buffer interface. Wraps backend buffer + allocation, exposes mapping,
///        GPU virtual address, and bindless table indices.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIResourceDesc.h"

#include <cstdint>

namespace Hylux::RHI
{

/// @brief Owning handle to a GPU buffer resource. Lifetime managed via Hylux::Ref<IRHIBuffer>.
class IRHIBuffer : public IRHIObject
{
public:
    /// @brief Returns the descriptor the buffer was created with.
    [[nodiscard]] virtual const BufferDesc& GetDesc() const noexcept = 0;

    /// @brief Returns the buffer size in bytes.
    [[nodiscard]] virtual std::uint64_t GetSize() const noexcept = 0;

    /// @brief Returns the GPU virtual address of the buffer base, or 0 if the backend does
    ///        not expose addresses (e.g. when BufferUsage::ShaderDeviceAddress is not set).
    [[nodiscard]] virtual std::uint64_t GetGpuAddress() const noexcept = 0;

    /// @brief Maps a CPU-visible sub-range of the buffer. Returns nullptr on failure.
    /// @param offset Byte offset from the start of the buffer.
    /// @param size   Byte count to map, or kWholeSize for the remainder.
    virtual void* Map(std::uint64_t offset = 0, std::uint64_t size = kWholeSize) = 0;

    /// @brief Releases the active mapping established by Map.
    virtual void Unmap() = 0;

    /// @brief Returns the bindless table index for the requested kind, or BindlessIndex::Invalid
    ///        if the buffer was created without the corresponding bindless flag.
    [[nodiscard]] virtual BindlessIndex GetBindlessIndex(BindlessKind kind) const noexcept = 0;
};

} // namespace Hylux::RHI
