/// @file
/// @brief Reflected enum descriptor (value <-> name bidirectional mapping).

#pragma once

#include "Core/Reflection/TypeId.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Hylux
{

/// @brief Holds reflection metadata for a single enum (scoped or unscoped).
class EnumInfo
{
public:
    struct Entry
    {
        std::string name;
        std::int64_t value;
    };

    EnumInfo(TypeId id, std::string name, std::size_t size) noexcept;

    [[nodiscard]] TypeId Id() const noexcept { return id_; }
    [[nodiscard]] const std::string& Name() const noexcept { return name_; }
    [[nodiscard]] std::size_t Size() const noexcept { return size_; }
    [[nodiscard]] const std::vector<Entry>& Entries() const noexcept { return entries_; }

    void AddValue(std::string valueName, std::int64_t value);

    [[nodiscard]] std::optional<std::string_view> NameOf(std::int64_t value) const noexcept;
    [[nodiscard]] std::optional<std::int64_t> ValueOf(std::string_view valueName) const noexcept;

private:
    TypeId id_{TypeId::Invalid};
    std::string name_;
    std::size_t size_{0};
    std::vector<Entry> entries_;
};

} // namespace Hylux
