/// @file
/// @brief TypeRegistry implementation.

#include "Core/Reflection/TypeRegistry.h"

#include <utility>

namespace Hylux
{

TypeRegistry& TypeRegistry::Get() noexcept
{
    static TypeRegistry instance;
    return instance;
}

TypeInfo* TypeRegistry::Register(TypeInfo info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const std::uint64_t key = ToU64(info.Id());
    auto existing = typesById_.find(key);
    if (existing != typesById_.end())
    {
        return &existing->second;
    }
    auto [it, inserted] = typesById_.emplace(key, std::move(info));
    typesByName_.emplace(it->second.Name(), &it->second);
    return &it->second;
}

EnumInfo* TypeRegistry::Register(EnumInfo info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const std::uint64_t key = ToU64(info.Id());
    auto existing = enumsById_.find(key);
    if (existing != enumsById_.end())
    {
        return &existing->second;
    }
    auto [it, inserted] = enumsById_.emplace(key, std::move(info));
    enumsByName_.emplace(it->second.Name(), &it->second);
    return &it->second;
}

const TypeInfo* TypeRegistry::FindType(TypeId id) const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = typesById_.find(ToU64(id));
    return it != typesById_.end() ? &it->second : nullptr;
}

const TypeInfo* TypeRegistry::FindType(std::string_view typeName) const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = typesByName_.find(std::string(typeName));
    return it != typesByName_.end() ? it->second : nullptr;
}

const EnumInfo* TypeRegistry::FindEnum(TypeId id) const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = enumsById_.find(ToU64(id));
    return it != enumsById_.end() ? &it->second : nullptr;
}

const EnumInfo* TypeRegistry::FindEnum(std::string_view enumName) const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = enumsByName_.find(std::string(enumName));
    return it != enumsByName_.end() ? it->second : nullptr;
}

std::vector<const TypeInfo*> TypeRegistry::AllTypes() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<const TypeInfo*> out;
    out.reserve(typesById_.size());
    for (const auto& [id, info] : typesById_)
    {
        out.push_back(&info);
    }
    return out;
}

std::vector<const EnumInfo*> TypeRegistry::AllEnums() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<const EnumInfo*> out;
    out.reserve(enumsById_.size());
    for (const auto& [id, info] : enumsById_)
    {
        out.push_back(&info);
    }
    return out;
}

namespace Reflection::Detail
{

AutoRegister::AutoRegister(Factory factory)
{
    TypeRegistry::Get().Register(factory());
}

AutoRegisterEnum::AutoRegisterEnum(Factory factory)
{
    TypeRegistry::Get().Register(factory());
}

} // namespace Reflection::Detail
} // namespace Hylux
