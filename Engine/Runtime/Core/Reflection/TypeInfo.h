/// @file
/// @brief Reflection metadata for a single C++ type.

#pragma once

#include "Core/Reflection/Field.h"
#include "Core/Reflection/Method.h"
#include "Core/Reflection/TypeId.h"

#include <cassert>
#include <string>
#include <string_view>
#include <vector>

namespace Hylux
{

using DefaultCtorFn = void* (*)();
using DtorFn = void (*)(void*);
using CopyCtorFn = void* (*)(const void*);
using MoveCtorFn = void* (*)(void*);

/// @brief Holds reflection metadata for a single class or struct.
class TypeInfo
{
public:
    TypeInfo(TypeId id, std::string name, std::size_t size, std::size_t alignment) noexcept;

    [[nodiscard]] TypeId Id() const noexcept { return id_; }
    [[nodiscard]] const std::string& Name() const noexcept { return name_; }
    [[nodiscard]] std::size_t Size() const noexcept { return size_; }
    [[nodiscard]] std::size_t Alignment() const noexcept { return alignment_; }
    [[nodiscard]] const TypeInfo* Base() const noexcept { return baseType_; }
    [[nodiscard]] const std::vector<Field>& Fields() const noexcept { return fields_; }
    [[nodiscard]] const std::vector<Method>& Methods() const noexcept { return methods_; }

    [[nodiscard]] bool IsA(const TypeInfo* other) const noexcept;
    [[nodiscard]] bool IsDefaultConstructible() const noexcept { return defaultCtor_ != nullptr; }
    [[nodiscard]] bool IsCopyConstructible() const noexcept { return copyCtor_ != nullptr; }
    [[nodiscard]] bool IsMoveConstructible() const noexcept { return moveCtor_ != nullptr; }

    [[nodiscard]] void* NewInstance() const;
    [[nodiscard]] void* CopyInstance(const void* source) const;
    [[nodiscard]] void* MoveInstance(void* source) const;
    void Destroy(void* instance) const;

    [[nodiscard]] const Field* FindField(std::string_view fieldName) const noexcept;
    [[nodiscard]] const Method* FindMethod(std::string_view methodName) const noexcept;

    /// @brief Typed convenience wrapper around NewInstance for default-constructible T.
    template<typename T> [[nodiscard]] T* Construct() const
    {
        assert(id_ == TypeIdOf<T>() && "TypeInfo::Construct<T>() TypeId mismatch");
        return static_cast<T*>(NewInstance());
    }

    void SetBase(const TypeInfo* base) noexcept { baseType_ = base; }
    void SetLifecycle(DefaultCtorFn defaultCtor, DtorFn dtor, CopyCtorFn copyCtor, MoveCtorFn moveCtor) noexcept;
    void AddField(Field field);
    void AddMethod(Method method);

private:
    TypeId id_{TypeId::Invalid};
    std::string name_;
    std::size_t size_{0};
    std::size_t alignment_{0};
    const TypeInfo* baseType_{nullptr};
    DefaultCtorFn defaultCtor_{nullptr};
    DtorFn dtor_{nullptr};
    CopyCtorFn copyCtor_{nullptr};
    MoveCtorFn moveCtor_{nullptr};
    std::vector<Field> fields_;
    std::vector<Method> methods_;
};

/// @brief Safe down-cast that consults the reflected base chain.
template<typename T, typename From> [[nodiscard]] T* Cast(From* instance) noexcept
{
    if (instance == nullptr)
    {
        return nullptr;
    }
    const TypeInfo* targetType = T::GetStaticType();
    if (const TypeInfo* actualType = instance->GetType();
        actualType != nullptr && targetType != nullptr && actualType->IsA(targetType))
    {
        return static_cast<T*>(instance);
    }
    return nullptr;
}

template<typename T, typename From> [[nodiscard]] const T* Cast(const From* instance) noexcept
{
    if (instance == nullptr)
    {
        return nullptr;
    }
    const TypeInfo* targetType = T::GetStaticType();
    if (const TypeInfo* actualType = instance->GetType();
        actualType != nullptr && targetType != nullptr && actualType->IsA(targetType))
    {
        return static_cast<const T*>(instance);
    }
    return nullptr;
}

} // namespace Hylux
