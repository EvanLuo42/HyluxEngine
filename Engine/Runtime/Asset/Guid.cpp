/// @file
/// @brief Guid implementation. Parsing/formatting follow RFC-4122 canonical 8-4-4-4-12
///        layout; Generate() uses std::random_device-seeded mt19937_64 to fill 16 random
///        bytes, then patches version (v4) and variant (RFC) bits per the spec.

#include "Asset/Guid.h"

#include <cstring>
#include <random>

namespace Hylux::Asset
{
namespace
{

constexpr std::size_t kCanonicalLength = 36;

bool HexDigit(char c, std::uint8_t& outNibble) noexcept
{
    if (c >= '0' && c <= '9')
    {
        outNibble = static_cast<std::uint8_t>(c - '0');
        return true;
    }
    if (c >= 'a' && c <= 'f')
    {
        outNibble = static_cast<std::uint8_t>(c - 'a' + 10);
        return true;
    }
    if (c >= 'A' && c <= 'F')
    {
        outNibble = static_cast<std::uint8_t>(c - 'A' + 10);
        return true;
    }
    return false;
}

bool ReadHexByte(std::string_view text, std::size_t pos, std::uint8_t& outByte) noexcept
{
    std::uint8_t hi = 0;
    std::uint8_t lo = 0;
    if (!HexDigit(text[pos], hi) || !HexDigit(text[pos + 1], lo))
    {
        return false;
    }
    outByte = static_cast<std::uint8_t>((hi << 4) | lo);
    return true;
}

char NibbleToHex(std::uint8_t nibble) noexcept
{
    return nibble < 10 ? static_cast<char>('0' + nibble) : static_cast<char>('a' + nibble - 10);
}

void WriteHexByte(std::uint8_t byte, std::string& out)
{
    out.push_back(NibbleToHex(static_cast<std::uint8_t>(byte >> 4)));
    out.push_back(NibbleToHex(static_cast<std::uint8_t>(byte & 0x0Fu)));
}

std::mt19937_64& ThreadLocalRng()
{
    thread_local std::mt19937_64 rng{std::random_device{}()};
    return rng;
}

} // namespace

Guid Guid::Generate()
{
    Guid out{};
    auto& rng = ThreadLocalRng();
    const std::uint64_t a = rng();
    const std::uint64_t b = rng();
    std::memcpy(out.bytes.data(),     &a, sizeof(a));
    std::memcpy(out.bytes.data() + 8, &b, sizeof(b));

    out.bytes[6] = static_cast<std::uint8_t>((out.bytes[6] & 0x0Fu) | 0x40u);
    out.bytes[8] = static_cast<std::uint8_t>((out.bytes[8] & 0x3Fu) | 0x80u);
    return out;
}

std::optional<Guid> Guid::Parse(std::string_view text)
{
    if (text.size() != kCanonicalLength)
    {
        return std::nullopt;
    }
    if (text[8] != '-' || text[13] != '-' || text[18] != '-' || text[23] != '-')
    {
        return std::nullopt;
    }

    constexpr std::size_t kHexPositions[16] = {
        0, 2, 4, 6,
        9, 11,
        14, 16,
        19, 21,
        24, 26, 28, 30, 32, 34,
    };

    Guid out{};
    for (std::size_t i = 0; i < kSize; ++i)
    {
        if (!ReadHexByte(text, kHexPositions[i], out.bytes[i]))
        {
            return std::nullopt;
        }
    }
    return out;
}

std::string Guid::ToString() const
{
    std::string out;
    out.reserve(kCanonicalLength);

    auto write = [&](std::size_t first, std::size_t count) {
        for (std::size_t i = 0; i < count; ++i)
        {
            WriteHexByte(bytes[first + i], out);
        }
    };

    write(0, 4);
    out.push_back('-');
    write(4, 2);
    out.push_back('-');
    write(6, 2);
    out.push_back('-');
    write(8, 2);
    out.push_back('-');
    write(10, 6);
    return out;
}

bool Guid::IsZero() const noexcept
{
    for (auto b : bytes)
    {
        if (b != 0)
        {
            return false;
        }
    }
    return true;
}

std::uint64_t Guid::Hash() const noexcept
{
    std::uint64_t lo = 0;
    std::uint64_t hi = 0;
    std::memcpy(&lo, bytes.data(),     sizeof(lo));
    std::memcpy(&hi, bytes.data() + 8, sizeof(hi));
    return lo ^ ((hi << 1) | (hi >> 63));
}

} // namespace Hylux::Asset
