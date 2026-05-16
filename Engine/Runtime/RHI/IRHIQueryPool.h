/// @file
/// @brief Query pool interface for timestamps, occlusion, and pipeline statistics.

#pragma once

#include "RHI/IRHIObject.h"
#include "RHI/RHIEnums.h"

#include <cstdint>
#include <span>

namespace Hylux::RHI
{

/// @brief Reference-counted handle to a pool of GPU queries of a single kind.
class IRHIQueryPool : public IRHIObject
{
public:
    /// @brief Returns the kind of queries the pool was created with.
    [[nodiscard]] virtual QueryType GetType() const noexcept = 0;

    /// @brief Returns the number of query slots in the pool.
    [[nodiscard]] virtual std::uint32_t GetCount() const noexcept = 0;

    /// @brief Reads back the requested range of query results into a host-visible array.
    ///        For timestamps, results are 64-bit ticks. Returns false if any query is
    ///        unavailable. Use IRHICommandList::ResolveQueries for GPU-side resolve.
    virtual bool GetResults(std::uint32_t firstIndex, std::uint32_t count,
                            std::span<std::uint64_t> outResults, bool wait) = 0;
};

} // namespace Hylux::RHI
