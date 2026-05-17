/// @file
/// @brief RAII handle to a contiguous read-only byte region. The region may live in a
///        memory-mapped file view or in a heap buffer; the consumer only sees Bytes().

#pragma once

#include <cstddef>
#include <span>
#include <string_view>

namespace Hylux
{

/// @brief Reference to an immutable byte range owned by an IBlobStore backend. The bytes
///        remain valid for the lifetime of this handle. Implementations are not required
///        to be thread-safe; callers must serialize access externally.
class IMappedBlob
{
public:
    virtual ~IMappedBlob() = default;

    IMappedBlob(const IMappedBlob&)            = delete;
    IMappedBlob& operator=(const IMappedBlob&) = delete;
    IMappedBlob(IMappedBlob&&)                 = delete;
    IMappedBlob& operator=(IMappedBlob&&)      = delete;

    /// @brief Returns the mapped byte range. Empty span if the blob is zero-length.
    [[nodiscard]] virtual std::span<const std::byte> Bytes() const noexcept = 0;

    /// @brief Returns a diagnostic identifier (e.g. absolute file path or VFS URI). Used
    ///        only for logging and error messages.
    [[nodiscard]] virtual std::string_view DebugSource() const noexcept = 0;

protected:
    IMappedBlob() = default;
};

} // namespace Hylux
