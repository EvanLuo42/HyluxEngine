/// @file
/// @brief ProxyRegistry implementation.

#include "Renderer/Proxy/ProxyRegistry.h"

namespace Hylux::Renderer
{

PrimitiveProxy& ProxyRegistry::Spawn(const ProxyId id, const std::uint32_t transformIndex,
                                     const std::uint32_t layerMask)
{
    auto [it, inserted] = primitives_.try_emplace(id);
    PrimitiveProxy& proxy = it->second;
    proxy.id = id;
    proxy.transformIndex = transformIndex;
    proxy.layerMask = layerMask;
    proxy.flags = 0;
    proxy.meshHandle = 0;
    proxy.materialProxy = nullptr;
    proxy.bounds = {};
    return proxy;
}

void ProxyRegistry::Despawn(const ProxyId id)
{
    primitives_.erase(id);
}

void ProxyRegistry::AssignMaterial(const ProxyId id, MaterialProxy* materialProxy)
{
    if (const auto it = primitives_.find(id); it != primitives_.end())
    {
        it->second.materialProxy = materialProxy;
    }
}

void ProxyRegistry::AssignMesh(const ProxyId id, const std::uint64_t meshHandle)
{
    if (const auto it = primitives_.find(id); it != primitives_.end())
    {
        it->second.meshHandle = meshHandle;
    }
}

void ProxyRegistry::SetFlags(const ProxyId id, const std::uint32_t flags)
{
    if (const auto it = primitives_.find(id); it != primitives_.end())
    {
        it->second.flags = flags;
    }
}

void ProxyRegistry::SetBounds(const ProxyId id, const PrimitiveBounds& bounds)
{
    if (const auto it = primitives_.find(id); it != primitives_.end())
    {
        it->second.bounds = bounds;
    }
}

const PrimitiveProxy* ProxyRegistry::Find(const ProxyId id) const noexcept
{
    const auto it = primitives_.find(id);
    return it == primitives_.end() ? nullptr : &it->second;
}

} // namespace Hylux::Renderer
