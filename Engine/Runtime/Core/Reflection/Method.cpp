/// @file
/// @brief Method non-template members.

#include "Core/Reflection/Method.h"

#include <cassert>
#include <utility>

namespace Hylux
{

Method::Method(std::string name, TypeId returnType, std::vector<TypeId> paramTypes, MethodInvoker invoker) noexcept
    : name_(std::move(name)), returnType_(returnType), paramTypes_(std::move(paramTypes)), invoker_(invoker)
{}

Any Method::Invoke(void* instance, const std::span<Any> args) const
{
    assert(invoker_ != nullptr && "Method::Invoke called on uninitialized Method");
    assert(args.size() == paramTypes_.size() && "Method::Invoke argument count mismatch");
    return invoker_(instance, args);
}

} // namespace Hylux
