/// @file
/// @brief Global type registry plus static-init helpers used by the reflection macros.

#pragma once

#include "Core/Reflection/EnumInfo.h"
#include "Core/Reflection/TypeId.h"
#include "Core/Reflection/TypeInfo.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Hylux
{

/// @brief Process-wide registry of reflected types and enums.
///        Singleton with construct-on-first-use semantics to avoid SIOF.
class TypeRegistry
{
public:
    [[nodiscard]] static TypeRegistry& Get() noexcept;

    /// @brief Inserts a TypeInfo. Idempotent: returns the canonical entry if the id already exists.
    TypeInfo* Register(TypeInfo info);
    EnumInfo* Register(EnumInfo info);

    [[nodiscard]] const TypeInfo* FindType(TypeId id) const noexcept;
    [[nodiscard]] const TypeInfo* FindType(std::string_view typeName) const noexcept;
    [[nodiscard]] const EnumInfo* FindEnum(TypeId id) const noexcept;
    [[nodiscard]] const EnumInfo* FindEnum(std::string_view enumName) const noexcept;

    [[nodiscard]] std::vector<const TypeInfo*> AllTypes() const;
    [[nodiscard]] std::vector<const EnumInfo*> AllEnums() const;

private:
    TypeRegistry() = default;
    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;

    mutable std::mutex mutex_;
    std::unordered_map<std::uint64_t, TypeInfo> typesById_;
    std::unordered_map<std::string, TypeInfo*> typesByName_;
    std::unordered_map<std::uint64_t, EnumInfo> enumsById_;
    std::unordered_map<std::string, EnumInfo*> enumsByName_;
};

namespace Reflection::Detail
{

/// @brief File-scope static registrar emitted by HYLUX_REFLECT_CLASS_END.
struct AutoRegister
{
    using Factory = TypeInfo (*)();
    explicit AutoRegister(Factory factory);
};

/// @brief File-scope static registrar emitted by HYLUX_REFLECT_ENUM_END.
struct AutoRegisterEnum
{
    using Factory = EnumInfo (*)();
    explicit AutoRegisterEnum(Factory factory);
};

} // namespace Reflection::Detail
} // namespace Hylux
