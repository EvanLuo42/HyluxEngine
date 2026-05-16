/// @file
/// @brief Reflected field descriptor: name, type, offset, flags, typed accessors.

#pragma once

#include "Core/Reflection/TypeId.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

namespace Hylux
{

/// @brief Per-field metadata flags. Bit-or combinable.
enum class FieldFlags : std::uint32_t
{
    None = 0,
    ReadOnly = 1u << 0,
    Transient = 1u << 1,
    Serialize = 1u << 2,
    EditorOnly = 1u << 3,
};

constexpr FieldFlags operator|(FieldFlags a, FieldFlags b) noexcept
{
    return static_cast<FieldFlags>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

constexpr FieldFlags operator&(FieldFlags a, FieldFlags b) noexcept
{
    return static_cast<FieldFlags>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr bool HasFlag(FieldFlags value, FieldFlags flag) noexcept
{
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0u;
}

/// @brief Describes a single reflected data member of a class.
class Field
{
public:
    Field(std::string name, TypeId type, std::size_t offset, FieldFlags flags) noexcept;

    [[nodiscard]] const std::string& Name() const noexcept { return name_; }
    [[nodiscard]] TypeId Type() const noexcept { return type_; }
    [[nodiscard]] std::size_t Offset() const noexcept { return offset_; }
    [[nodiscard]] FieldFlags Flags() const noexcept { return flags_; }

    [[nodiscard]] void* GetPtr(void* instance) const noexcept;
    [[nodiscard]] const void* GetPtr(const void* instance) const noexcept;

    /// @brief Returns a typed reference to the field inside instance. Asserts on type mismatch.
    template<typename T> [[nodiscard]] T& Get(void* instance) const
    {
        assert(TypeIdOf<std::remove_cv_t<T>>() == type_ && "Field::Get<T>() type mismatch");
        return *static_cast<T*>(GetPtr(instance));
    }

    template<typename T> [[nodiscard]] const T& Get(const void* instance) const
    {
        assert(TypeIdOf<std::remove_cv_t<T>>() == type_ && "Field::Get<T>() type mismatch");
        return *static_cast<const T*>(GetPtr(instance));
    }

    /// @brief Assigns value into the field. Asserts on type mismatch or ReadOnly.
    template<typename T> void Set(void* instance, const T& value) const
    {
        assert(TypeIdOf<std::remove_cv_t<T>>() == type_ && "Field::Set<T>() type mismatch");
        assert(!HasFlag(flags_, FieldFlags::ReadOnly) && "Field::Set on ReadOnly field");
        *static_cast<T*>(GetPtr(instance)) = value;
    }

private:
    std::string name_;
    TypeId type_{TypeId::Invalid};
    std::size_t offset_{0};
    FieldFlags flags_{FieldFlags::None};
};

} // namespace Hylux
