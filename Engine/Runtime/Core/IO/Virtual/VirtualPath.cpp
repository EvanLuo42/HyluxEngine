/// @file
/// @brief VirtualPath implementation.

#include "Core/IO/Virtual/VirtualPath.h"

#include "Core/Utils/Hash.h"

#include <vector>

namespace Hylux
{
namespace
{

constexpr char FoldLower(char c) noexcept
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
}

constexpr char FoldSep(char c) noexcept
{
    return c == '\\' ? '/' : c;
}

} // namespace

std::string VirtualPath::Normalize(std::string_view path)
{
    if (path.empty()) return {};

    const char first = FoldSep(path.front());
    if (first != '/') return {};

    std::vector<std::string_view> segments;
    segments.reserve(8);

    std::size_t i = 1;
    while (i < path.size())
    {
        while (i < path.size() && FoldSep(path[i]) == '/') ++i;
        const std::size_t start = i;
        while (i < path.size() && FoldSep(path[i]) != '/') ++i;
        if (start == i) break;

        const std::string_view seg = path.substr(start, i - start);
        if (seg == ".") continue;
        if (seg == "..")
        {
            if (segments.empty()) return {};
            segments.pop_back();
            continue;
        }
        segments.push_back(seg);
    }

    if (segments.empty()) return {};

    std::string out;
    out.reserve(path.size());
    for (const auto& seg : segments)
    {
        out.push_back('/');
        for (const char c : seg) out.push_back(FoldSep(c));
    }
    return out;
}

bool VirtualPath::Split(std::string_view normalizedPath,
                        std::string_view& outPrefix,
                        std::string_view& outSubPath) noexcept
{
    if (normalizedPath.size() < 2 || normalizedPath.front() != '/') return false;

    const std::size_t second = normalizedPath.find('/', 1);
    if (second == std::string_view::npos) return false;
    if (second == 1) return false;

    outPrefix  = normalizedPath.substr(0, second + 1);
    outSubPath = normalizedPath.substr(second + 1);
    return true;
}

std::string VirtualPath::ToLowerAscii(std::string_view text)
{
    std::string out;
    out.resize(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) out[i] = FoldLower(text[i]);
    return out;
}

std::uint64_t VirtualPath::HashLowerAscii(std::string_view path) noexcept
{
    std::uint64_t hash = Hash::Fnv1a64Offset;
    for (const char c : path)
    {
        hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(FoldLower(c)));
        hash *= Hash::Fnv1a64Prime;
    }
    return hash;
}

bool VirtualPath::IsValidMountPrefix(std::string_view prefix) noexcept
{
    if (prefix.size() < 3) return false;
    if (prefix.front() != '/' || prefix.back() != '/') return false;

    bool inSegment = false;
    std::size_t segmentLen = 0;
    for (std::size_t i = 0; i < prefix.size(); ++i)
    {
        const char c = prefix[i];
        if (c == '\\') return false;
        if (c == '/')
        {
            if (inSegment && segmentLen == 0) return false;
            inSegment  = true;
            segmentLen = 0;
        }
        else
        {
            ++segmentLen;
        }
    }
    return true;
}

} // namespace Hylux
