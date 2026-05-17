/// @file
/// @brief PermutationKeyBuilder implementation. Each Add* call hashes a category byte
///        followed by the argument bytes, so two pushes with the same payload but
///        different declared types end up in different keys.

#include "Shader/PermutationKey.h"

#include <cstring>

namespace Hylux::Shader
{

namespace
{

constexpr std::byte kTagBool   = std::byte{0x01};
constexpr std::byte kTagInt    = std::byte{0x02};
constexpr std::byte kTagTypeId = std::byte{0x03};
constexpr std::byte kTagRaw    = std::byte{0x04};

inline std::uint64_t HashByte(std::uint64_t seed, std::byte b) noexcept
{
    return Hash::Fnv1a64(reinterpret_cast<const char*>(&b), 1, seed);
}

inline std::uint64_t HashBytes(std::uint64_t seed, const void* data, std::size_t size) noexcept
{
    return Hash::Fnv1a64(static_cast<const char*>(data), size, seed);
}

} // namespace

void PermutationKeyBuilder::AddBool(bool value) noexcept
{
    hash_ = HashByte(hash_, kTagBool);
    const std::byte payload = value ? std::byte{0x01} : std::byte{0x00};
    hash_ = HashByte(hash_, payload);
}

void PermutationKeyBuilder::AddInt(std::int64_t value) noexcept
{
    hash_ = HashByte(hash_, kTagInt);
    std::uint64_t encoded = static_cast<std::uint64_t>(value);
    std::byte buffer[8];
    for (std::size_t i = 0; i < 8; ++i)
    {
        buffer[i] = static_cast<std::byte>(encoded & 0xFFu);
        encoded >>= 8;
    }
    hash_ = HashBytes(hash_, buffer, sizeof(buffer));
}

void PermutationKeyBuilder::AddTypeId(std::uint32_t stableTypeId) noexcept
{
    hash_ = HashByte(hash_, kTagTypeId);
    std::byte buffer[4];
    for (std::size_t i = 0; i < 4; ++i)
    {
        buffer[i] = static_cast<std::byte>(stableTypeId & 0xFFu);
        stableTypeId >>= 8;
    }
    hash_ = HashBytes(hash_, buffer, sizeof(buffer));
}

void PermutationKeyBuilder::AddRaw(std::span<const std::byte> bytes) noexcept
{
    hash_ = HashByte(hash_, kTagRaw);
    std::uint64_t lengthLe = bytes.size();
    std::byte lengthBuffer[8];
    for (std::size_t i = 0; i < 8; ++i)
    {
        lengthBuffer[i] = static_cast<std::byte>(lengthLe & 0xFFu);
        lengthLe >>= 8;
    }
    hash_ = HashBytes(hash_, lengthBuffer, sizeof(lengthBuffer));
    if (!bytes.empty())
    {
        hash_ = HashBytes(hash_, bytes.data(), bytes.size());
    }
}

} // namespace Hylux::Shader
