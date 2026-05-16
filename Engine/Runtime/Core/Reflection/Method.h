/// @file
/// @brief Reflected method descriptor with a templated Bind() factory.

#pragma once

#include "Core/Reflection/Any.h"
#include "Core/Reflection/TypeId.h"

#include <cassert>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Hylux
{

using MethodInvoker = Any (*)(void* instance, std::span<Any> args);

/// @brief Describes a single reflected member function.
class Method
{
public:
    Method(std::string name, TypeId returnType, std::vector<TypeId> paramTypes, MethodInvoker invoker) noexcept;

    [[nodiscard]] const std::string& Name() const noexcept { return name_; }
    [[nodiscard]] TypeId ReturnType() const noexcept { return returnType_; }
    [[nodiscard]] const std::vector<TypeId>& ParamTypes() const noexcept { return paramTypes_; }
    [[nodiscard]] std::size_t Arity() const noexcept { return paramTypes_.size(); }

    /// @brief Invokes the method on instance, passing args by reference through Any.
    Any Invoke(void* instance, std::span<Any> args) const;

    /// @brief Deduces signature from MemberPtr and produces a Method descriptor.
    template<auto MemberPtr> [[nodiscard]] static Method Bind(std::string name);

private:
    std::string name_;
    TypeId returnType_{TypeId::Invalid};
    std::vector<TypeId> paramTypes_;
    MethodInvoker invoker_{nullptr};
};

namespace Reflection::Detail
{

template<typename T> [[nodiscard]] inline T* AnyGetTypedArg(Any& arg) noexcept
{
    T* typed = arg.TryGet<T>();
    assert(typed != nullptr && "Method::Invoke argument TypeId mismatch");
    return typed;
}

template<typename Class, typename Ret, typename... Args, std::size_t... I>
inline Any InvokeMember(Ret (Class::*memberPtr)(Args...), void* instance, std::span<Any> args,
                        std::index_sequence<I...>)
{
    auto* self = static_cast<Class*>(instance);
    if constexpr (std::is_void_v<Ret>)
    {
        (self->*memberPtr)(*AnyGetTypedArg<std::decay_t<Args>>(args[I])...);
        return Any{};
    }
    else
    {
        Ret result = (self->*memberPtr)(*AnyGetTypedArg<std::decay_t<Args>>(args[I])...);
        return Any::Make<Ret>(std::move(result));
    }
}

template<typename Class, typename Ret, typename... Args, std::size_t... I>
inline Any InvokeMemberConst(Ret (Class::*memberPtr)(Args...) const, const void* instance, std::span<Any> args,
                             std::index_sequence<I...>)
{
    const auto* self = static_cast<const Class*>(instance);
    if constexpr (std::is_void_v<Ret>)
    {
        (self->*memberPtr)(*AnyGetTypedArg<std::decay_t<Args>>(args[I])...);
        return Any{};
    }
    else
    {
        Ret result = (self->*memberPtr)(*AnyGetTypedArg<std::decay_t<Args>>(args[I])...);
        return Any::Make<Ret>(std::move(result));
    }
}

template<auto MemberPtr> struct MethodTraits;

template<typename Class, typename Ret, typename... Args, Ret (Class::*Ptr)(Args...)> struct MethodTraits<Ptr>
{
    using ReturnType = Ret;
    static constexpr std::size_t Arity = sizeof...(Args);

    static Any Invoke(void* instance, std::span<Any> args)
    {
        return InvokeMember<Class, Ret, Args...>(Ptr, instance, args, std::index_sequence_for<Args...>{});
    }

    static std::vector<TypeId> ParamTypeIds() { return {TypeIdOf<std::decay_t<Args>>()...}; }
};

template<typename Class, typename Ret, typename... Args, Ret (Class::*Ptr)(Args...) const> struct MethodTraits<Ptr>
{
    using ReturnType = Ret;
    static constexpr std::size_t Arity = sizeof...(Args);

    static Any Invoke(void* instance, std::span<Any> args)
    {
        return InvokeMemberConst<Class, Ret, Args...>(Ptr, instance, args, std::index_sequence_for<Args...>{});
    }

    static std::vector<TypeId> ParamTypeIds() { return {TypeIdOf<std::decay_t<Args>>()...}; }
};

} // namespace Reflection::Detail

template<auto MemberPtr> Method Method::Bind(std::string name)
{
    using Traits = Reflection::Detail::MethodTraits<MemberPtr>;
    return Method(std::move(name), TypeIdOf<std::decay_t<typename Traits::ReturnType>>(), Traits::ParamTypeIds(),
                  &Traits::Invoke);
}

} // namespace Hylux
