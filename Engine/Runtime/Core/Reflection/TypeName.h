/// @file
/// @brief Compile-time type-name extraction based on MSVC's __FUNCSIG__.

#pragma once

#include <string_view>

namespace Hylux::Reflection::Detail
{

/// @brief Probe whose signature contains T in a predictable position.
template<typename T> constexpr std::string_view TypeNameProbe() noexcept
{
    return __FUNCSIG__;
}

/// @brief Returns a stable, MSVC-canonical name for T as a string_view into the probe's signature.
template<typename T> constexpr std::string_view TypeName() noexcept
{
    constexpr std::string_view sig = TypeNameProbe<T>();
    constexpr std::string_view prefixToken = "TypeNameProbe<";
    constexpr std::string_view suffixToken = ">(void) noexcept";

    const auto prefixPos = sig.find(prefixToken);
    const auto start = prefixPos + prefixToken.size();
    const auto end = sig.rfind(suffixToken);
    std::string_view name = sig.substr(start, end - start);

    constexpr std::string_view classKw = "class ";
    constexpr std::string_view structKw = "struct ";
    constexpr std::string_view enumKw = "enum ";
    constexpr std::string_view unionKw = "union ";
    if (name.substr(0, classKw.size()) == classKw)
    {
        name.remove_prefix(classKw.size());
    }
    else if (name.substr(0, structKw.size()) == structKw)
    {
        name.remove_prefix(structKw.size());
    }
    else if (name.substr(0, enumKw.size()) == enumKw)
    {
        name.remove_prefix(enumKw.size());
    }
    else if (name.substr(0, unionKw.size()) == unionKw)
    {
        name.remove_prefix(unionKw.size());
    }
    return name;
}

} // namespace Hylux::Reflection::Detail

namespace Hylux
{

/// @brief Public type-name accessor. Forwards to the detail probe.
template<typename T> [[nodiscard]] constexpr std::string_view TypeName() noexcept
{
    return Reflection::Detail::TypeName<T>();
}

} // namespace Hylux
