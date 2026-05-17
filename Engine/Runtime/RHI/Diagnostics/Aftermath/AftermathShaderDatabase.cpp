/// @file
/// @brief AftermathShaderDatabase implementation.

#include "RHI/Diagnostics/Aftermath/AftermathShaderDatabase.h"

namespace Hylux::RHI::Vulkan
{

void AftermathShaderDatabase::Register(const AftermathShaderEntry& entry)
{
    std::lock_guard<std::mutex> lock(mutex_);
    entries_[entry.shaderHash] = entry;  // overwrites prior registration for the same hash
}

const AftermathShaderEntry* AftermathShaderDatabase::Find(std::uint64_t hash) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = entries_.find(hash);
    return it == entries_.end() ? nullptr : &it->second;
}

void AftermathShaderDatabase::Clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

} // namespace Hylux::RHI::Vulkan
