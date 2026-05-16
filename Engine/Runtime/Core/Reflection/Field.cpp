/// @file
/// @brief Field non-template members.

#include "Core/Reflection/Field.h"

#include <cstddef>
#include <utility>

namespace Hylux
{

Field::Field(std::string name, TypeId type, std::size_t offset, FieldFlags flags) noexcept
    : name_(std::move(name)), type_(type), offset_(offset), flags_(flags)
{}

void* Field::GetPtr(void* instance) const noexcept
{
    return static_cast<std::byte*>(instance) + offset_;
}

const void* Field::GetPtr(const void* instance) const noexcept
{
    return static_cast<const std::byte*>(instance) + offset_;
}

} // namespace Hylux
