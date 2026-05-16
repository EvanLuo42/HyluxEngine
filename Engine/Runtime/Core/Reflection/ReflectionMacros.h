/// @file
/// @brief Intrusive macros used to declare and register reflected types and enums.

#pragma once

#include "Core/Reflection/Field.h"
#include "Core/Reflection/Method.h"
#include "Core/Reflection/TypeId.h"
#include "Core/Reflection/TypeInfo.h"
#include "Core/Reflection/TypeName.h"
#include "Core/Reflection/TypeRegistry.h"

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#define HYLUX_REFL_CAT_(a, b) a##b
#define HYLUX_REFL_CAT(a, b) HYLUX_REFL_CAT_(a, b)
#define HYLUX_REFL_UNIQUE(prefix) HYLUX_REFL_CAT(prefix, __LINE__)

/// @brief Declares reflection hooks inside a polymorphic class body (adds virtual GetType()).
///        Insert at the top of the class body; the macro ends with private:, so add your own
///        access specifier afterwards.
#define HYLUX_REFLECT_BODY(ClassName)                                                                                  \
public:                                                                                                                \
    using HyluxReflectSelf = ClassName;                                                                                \
    static const ::Hylux::TypeInfo* GetStaticType() noexcept;                                                          \
    virtual const ::Hylux::TypeInfo* GetType() const noexcept                                                          \
    {                                                                                                                  \
        return ClassName::GetStaticType();                                                                             \
    }                                                                                                                  \
                                                                                                                       \
private:

/// @brief Same as HYLUX_REFLECT_BODY but does not add a virtual GetType(); use for value types
///        that must not gain a vtable.
#define HYLUX_REFLECT_BODY_POD(ClassName)                                                                              \
public:                                                                                                                \
    using HyluxReflectSelf = ClassName;                                                                                \
    static const ::Hylux::TypeInfo* GetStaticType() noexcept;                                                          \
                                                                                                                       \
private:

/// @brief Opens a reflection registration block for ClassName inside a .cpp.
///        Pair with HYLUX_REFLECT_CLASS_END(ClassName). Pass the fully-qualified class name when
///        the class lives inside a namespace.
#define HYLUX_REFLECT_CLASS_BEGIN(ClassName)                                                                           \
    static const ::Hylux::Reflection::Detail::AutoRegister HYLUX_REFL_UNIQUE(HyluxAutoReg_)                            \
    {                                                                                                                  \
        +[]() -> ::Hylux::TypeInfo {                                                               \
            using Self = ClassName;                                                                \
            ::Hylux::TypeInfo info(::Hylux::TypeIdOf<Self>(),                                      \
                                   std::string(::Hylux::TypeName<Self>()), sizeof(Self),           \
                                   alignof(Self));                                                 \
            info.SetLifecycle(                                                                     \
                +[]() -> void* {                                                                   \
                    if constexpr (std::is_default_constructible_v<Self>)                           \
                    {                                                                              \
                        return new Self();                                                         \
                    }                                                                              \
                    else                                                                           \
                    {                                                                              \
                        return nullptr;                                                            \
                    }                                                                              \
                },                                                                                 \
                +[](void* instance) { delete static_cast<Self*>(instance); },                      \
                +[](const void* source) -> void* {                                                 \
                    if constexpr (std::is_copy_constructible_v<Self>)                              \
                    {                                                                              \
                        return new Self(*static_cast<const Self*>(source));                        \
                    }                                                                              \
                    else                                                                           \
                    {                                                                              \
                        return nullptr;                                                            \
                    }                                                                              \
                },                                                                                 \
                +[](void* source) -> void* {                                                       \
                    if constexpr (std::is_move_constructible_v<Self>)                              \
                    {                                                                              \
                        return new Self(std::move(*static_cast<Self*>(source)));                   \
                    }                                                                              \
                    else                                                                           \
                    {                                                                              \
                        return nullptr;                                                            \
                    }                                                                              \
                });
/// @brief Links the current type's base class. Base must itself be reflected.
#define HYLUX_REFLECT_BASE(BaseName) info.SetBase(BaseName::GetStaticType());

/// @brief Registers a data member by identifier.
#define HYLUX_REFLECT_FIELD(FieldId)                                                                                   \
    info.AddField(::Hylux::Field(#FieldId,                                                                             \
                                 ::Hylux::TypeIdOf<std::remove_cvref_t<decltype(std::declval<Self&>().FieldId)>>(),    \
                                 offsetof(Self, FieldId), ::Hylux::FieldFlags::None));

/// @brief Registers a data member with explicit FieldFlags.
#define HYLUX_REFLECT_FIELD_FLAGS(FieldId, Flags)                                                                      \
    info.AddField(::Hylux::Field(#FieldId,                                                                             \
                                 ::Hylux::TypeIdOf<std::remove_cvref_t<decltype(std::declval<Self&>().FieldId)>>(),    \
                                 offsetof(Self, FieldId), (Flags)));

/// @brief Registers a member function by identifier.
#define HYLUX_REFLECT_METHOD(MethodId) info.AddMethod(::Hylux::Method::Bind<&Self::MethodId>(#MethodId));

/// @brief Closes a HYLUX_REFLECT_CLASS_BEGIN block and emits ClassName::GetStaticType().
#define HYLUX_REFLECT_CLASS_END(ClassName)                                                                             \
    return info;                                                                                                       \
    }                                                                                                                  \
    }                                                                                                                  \
    ;                                                                                                                  \
    const ::Hylux::TypeInfo* ClassName::GetStaticType() noexcept                                                       \
    {                                                                                                                  \
        return ::Hylux::TypeRegistry::Get().FindType(::Hylux::TypeIdOf<ClassName>());                                  \
    }

/// @brief Opens an enum reflection registration block for EnumName inside a .cpp.
#define HYLUX_REFLECT_ENUM_BEGIN(EnumName)                                                                             \
    static const ::Hylux::Reflection::Detail::AutoRegisterEnum HYLUX_REFL_UNIQUE(HyluxAutoRegEnum_)                    \
    {                                                                                                                  \
        +[]() -> ::Hylux::EnumInfo {                                            \
        using Self = EnumName;                                                                     \
        ::Hylux::EnumInfo info(::Hylux::TypeIdOf<Self>(),                                          \
                               std::string(::Hylux::TypeName<Self>()), sizeof(Self));
/// @brief Registers a single enumerator.
#define HYLUX_REFLECT_ENUM_VALUE(Value) info.AddValue(#Value, static_cast<std::int64_t>(Self::Value));

/// @brief Closes a HYLUX_REFLECT_ENUM_BEGIN block.
#define HYLUX_REFLECT_ENUM_END()                                                                                       \
    return info;                                                                                                       \
    }                                                                                                                  \
    }                                                                                                                  \
    ;
