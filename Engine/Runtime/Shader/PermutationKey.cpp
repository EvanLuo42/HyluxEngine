/// @file
/// @brief PermutationKeyBuilder implementation. Each Add* call hashes a category byte
///        followed by the argument bytes, so two pushes with the same payload but
///        different declared types end up in different keys.

#include "Shader/PermutationKey.h"

namespace Hylux::Shader
{

namespace
{

constexpr std::uint8_t kTagBool   = 0x01;
constexpr std::uint8_t kTagInt    = 0x02;
constexpr std::uint8_t kTagTypeId = 0x03;
constexpr std::uint8_t kTagRaw    = 0x04;

} // namespace

void PermutationKeyBuilder::AddBool(bool value) noexcept
{
    hash_ = Hash::MixU8(hash_, kTagBool);
    hash_ = Hash::MixU8(hash_, value ? 0x01u : 0x00u);
}

void PermutationKeyBuilder::AddInt(std::int64_t value) noexcept
{
    hash_ = Hash::MixU8(hash_, kTagInt);
    hash_ = Hash::MixU64(hash_, static_cast<std::uint64_t>(value));
}

void PermutationKeyBuilder::AddTypeId(std::uint32_t stableTypeId) noexcept
{
    hash_ = Hash::MixU8(hash_, kTagTypeId);
    hash_ = Hash::MixU32(hash_, stableTypeId);
}

void PermutationKeyBuilder::AddRaw(std::span<const std::byte> bytes) noexcept
{
    hash_ = Hash::MixU8(hash_, kTagRaw);
    hash_ = Hash::MixU64(hash_, static_cast<std::uint64_t>(bytes.size()));
    if (!bytes.empty())
    {
        hash_ = Hash::MixBytes(hash_, bytes.data(), bytes.size());
    }
}

} // namespace Hylux::Shader
