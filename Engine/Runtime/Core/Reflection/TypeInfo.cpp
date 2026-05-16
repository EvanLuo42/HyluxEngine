/// @file
/// @brief TypeInfo implementation.

#include "Core/Reflection/TypeInfo.h"

#include <utility>

namespace Hylux
{

TypeInfo::TypeInfo(TypeId id, std::string name, std::size_t size, std::size_t alignment) noexcept
    : id_(id), name_(std::move(name)), size_(size), alignment_(alignment)
{
}

bool TypeInfo::IsA(const TypeInfo* other) const noexcept
{
    if (other == nullptr)
    {
        return false;
    }
    for (const TypeInfo* current = this; current != nullptr; current = current->baseType_)
    {
        if (current == other || current->id_ == other->id_)
        {
            return true;
        }
    }
    return false;
}

void* TypeInfo::NewInstance() const
{
    return defaultCtor_ != nullptr ? defaultCtor_() : nullptr;
}

void* TypeInfo::CopyInstance(const void* source) const
{
    return copyCtor_ != nullptr ? copyCtor_(source) : nullptr;
}

void* TypeInfo::MoveInstance(void* source) const
{
    return moveCtor_ != nullptr ? moveCtor_(source) : nullptr;
}

void TypeInfo::Destroy(void* instance) const
{
    if (dtor_ != nullptr && instance != nullptr)
    {
        dtor_(instance);
    }
}

const Field* TypeInfo::FindField(std::string_view fieldName) const noexcept
{
    for (const Field& field : fields_)
    {
        if (field.Name() == fieldName)
        {
            return &field;
        }
    }
    if (baseType_ != nullptr)
    {
        return baseType_->FindField(fieldName);
    }
    return nullptr;
}

const Method* TypeInfo::FindMethod(std::string_view methodName) const noexcept
{
    for (const Method& method : methods_)
    {
        if (method.Name() == methodName)
        {
            return &method;
        }
    }
    if (baseType_ != nullptr)
    {
        return baseType_->FindMethod(methodName);
    }
    return nullptr;
}

void TypeInfo::SetLifecycle(DefaultCtorFn defaultCtor, DtorFn dtor, CopyCtorFn copyCtor,
                            MoveCtorFn moveCtor) noexcept
{
    defaultCtor_ = defaultCtor;
    dtor_ = dtor;
    copyCtor_ = copyCtor;
    moveCtor_ = moveCtor;
}

void TypeInfo::AddField(Field field)
{
    fields_.push_back(std::move(field));
}

void TypeInfo::AddMethod(Method method)
{
    methods_.push_back(std::move(method));
}

} // namespace Hylux
