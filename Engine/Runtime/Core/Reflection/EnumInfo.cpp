/// @file
/// @brief EnumInfo implementation.

#include "Core/Reflection/EnumInfo.h"

#include <utility>

namespace Hylux
{

EnumInfo::EnumInfo(const TypeId id, std::string name, const std::size_t size) noexcept
    : id_(id), name_(std::move(name)), size_(size)
{}

void EnumInfo::AddValue(std::string valueName, const std::int64_t value)
{
    entries_.push_back(Entry{std::move(valueName), value});
}

std::optional<std::string_view> EnumInfo::NameOf(const std::int64_t value) const noexcept
{
    for (const auto& [name, val] : entries_)
    {
        if (value == val)
        {
            return std::string_view{name};
        }
    }
    return std::nullopt;
}

std::optional<std::int64_t> EnumInfo::ValueOf(const std::string_view valueName) const noexcept
{
    for (const auto& [name, value] : entries_)
    {
        if (name == valueName)
        {
            return value;
        }
    }
    return std::nullopt;
}

} // namespace Hylux
