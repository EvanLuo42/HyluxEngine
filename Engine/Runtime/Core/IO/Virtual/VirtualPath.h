/// @file
/// @brief Virtual-path helpers: normalization, prefix/sub split, ASCII lowering, FNV-1a64 hashing.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Hylux
{

/// @brief Helpers for the Virtual File System path syntax.
///        Virtual paths use forward slashes and always begin with "/<Prefix>/...", e.g.
///        "/Engine/Shaders/Common.glsl". They are independent of any host filesystem and are
///        compared case-insensitively (ASCII fold) to match NTFS / UE behavior. Non-ASCII bytes
///        pass through untouched — they are treated as opaque UTF-8 and matched byte-exactly.
class VirtualPath
{
public:
    VirtualPath()                              = delete;
    VirtualPath(const VirtualPath&)            = delete;
    VirtualPath& operator=(const VirtualPath&) = delete;

    /// @brief Returns a normalized copy of @p path: collapses runs of '/', resolves "."/".." segments,
    ///        and validates the result. Returns empty string on failure (invalid input, escape attempt,
    ///        missing leading '/', or empty prefix). Backslashes are accepted and folded to '/'.
    [[nodiscard]] static std::string Normalize(std::string_view path);

    /// @brief Splits a *normalized* virtual path into its leading mount prefix (including both '/'s,
    ///        e.g. "/Engine/") and the remainder (e.g. "Shaders/Common.glsl"). Returns false if the
    ///        path is not a valid virtual path. The remainder is empty when @p path is exactly a prefix.
    [[nodiscard]] static bool Split(std::string_view normalizedPath,
                                    std::string_view& outPrefix,
                                    std::string_view& outSubPath) noexcept;

    /// @brief Returns a copy of @p text with ASCII 'A'..'Z' folded to 'a'..'z'. Non-ASCII bytes pass through.
    [[nodiscard]] static std::string ToLowerAscii(std::string_view text);

    /// @brief Computes the case-folded FNV-1a64 hash used for VFS path lookup.
    ///        Equivalent to Hash::Fnv1a64(ToLowerAscii(@p path)) but allocation-free.
    [[nodiscard]] static std::uint64_t HashLowerAscii(std::string_view path) noexcept;

    /// @brief Returns true iff @p prefix is a valid mount prefix: "/<Name>/", where Name is one or more
    ///        bytes with no '/' inside. Used by IVirtualFileSystem::Mount to reject malformed input.
    [[nodiscard]] static bool IsValidMountPrefix(std::string_view prefix) noexcept;
};

} // namespace Hylux
