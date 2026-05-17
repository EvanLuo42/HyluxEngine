/// @file
/// @brief MaterialInstance implementation.

#include "Renderer/Material/MaterialInstance.h"

#include "Core/Utils/Hash.h"

#include <cstring>

namespace Hylux::Renderer
{
namespace
{

std::uint64_t HashMix(std::uint64_t seed, const void* data, std::size_t size) noexcept
{
    return Hash::Fnv1a64(static_cast<const char*>(data), size, seed);
}

} // namespace

MaterialInstance::MaterialInstance(const MaterialAsset* asset) noexcept : asset_(asset) {}

std::uint64_t MaterialInstance::GetAssetHash() const noexcept
{
    return asset_ != nullptr ? asset_->GetAssetHash() : 0ull;
}

void MaterialInstance::MarkDirty() const noexcept
{
    hashDirty_ = true;
}

void MaterialInstance::SetScalar(NameHash name, float value)
{
    if (asset_ == nullptr)
    {
        return;
    }
    const auto* desc = asset_->FindParameter(name);
    if (desc == nullptr || desc->kind != ParameterKind::Scalar)
    {
        return;
    }
    ParameterValue& slot = overrides_[name];
    slot.kind = ParameterKind::Scalar;
    slot.vec[0] = value;
    MarkDirty();
}

void MaterialInstance::SetVector(NameHash name, float x, float y, float z, float w)
{
    if (asset_ == nullptr)
    {
        return;
    }
    const auto* desc = asset_->FindParameter(name);
    if (desc == nullptr || desc->kind != ParameterKind::Vector)
    {
        return;
    }
    ParameterValue& slot = overrides_[name];
    slot.kind = ParameterKind::Vector;
    slot.vec[0] = x;
    slot.vec[1] = y;
    slot.vec[2] = z;
    slot.vec[3] = w;
    MarkDirty();
}

void MaterialInstance::SetTexture(NameHash name, std::uint64_t textureHandle)
{
    if (asset_ == nullptr)
    {
        return;
    }
    const auto* desc = asset_->FindParameter(name);
    if (desc == nullptr || desc->kind != ParameterKind::Texture)
    {
        return;
    }
    ParameterValue& slot = overrides_[name];
    slot.kind = ParameterKind::Texture;
    slot.textureHandle = textureHandle;
    MarkDirty();
}

std::uint64_t MaterialInstance::GetInstanceHash() const noexcept
{
    if (!hashDirty_)
    {
        return cachedInstanceHash_;
    }
    std::uint64_t h = Hash::Fnv1a64Offset;
    if (asset_ != nullptr)
    {
        const auto assetHash = asset_->GetAssetHash();
        h = HashMix(h, &assetHash, sizeof(assetHash));
    }
    for (const auto& [name, value] : overrides_)
    {
        const auto nameRaw = ToU64(name);
        h = HashMix(h, &nameRaw, sizeof(nameRaw));
        h = HashMix(h, &value, sizeof(value));
    }
    cachedInstanceHash_ = h;

    // Recompute permutation key alongside; both depend on the same dirty bit.
    std::uint64_t perm = asset_ != nullptr ? asset_->GetPermutationBaseKey() : 0ull;
    if (asset_ != nullptr)
    {
        for (const auto& desc : asset_->GetParameters())
        {
            if (!desc.isPermutation)
            {
                continue;
            }
            const auto it = overrides_.find(desc.name);
            if (it == overrides_.end())
            {
                continue;
            }
            const auto nameRaw = ToU64(desc.name);
            perm = HashMix(perm, &nameRaw, sizeof(nameRaw));
            perm = HashMix(perm, &it->second, sizeof(it->second));
        }
    }
    cachedPermutationKey_ = perm;
    hashDirty_ = false;
    return cachedInstanceHash_;
}

std::uint64_t MaterialInstance::GetPermutationKey() const noexcept
{
    // Force recomputation through GetInstanceHash; the two hashes share a dirty bit.
    (void)GetInstanceHash();
    return cachedPermutationKey_;
}

MaterialSnapshot MaterialInstance::Snapshot() const
{
    MaterialSnapshot snap{};
    if (asset_ == nullptr)
    {
        return snap;
    }
    snap.materialAssetHash = asset_->GetAssetHash();
    snap.instanceHash = GetInstanceHash();
    snap.permutationKey = cachedPermutationKey_;

    snap.uniformBlock.assign(asset_->GetUniformBlockSize(), std::byte{0});
    snap.textureHandles.assign(asset_->GetTextureCount(), 0ull);

    for (const auto& desc : asset_->GetParameters())
    {
        const auto it = overrides_.find(desc.name);
        if (it == overrides_.end())
        {
            continue;
        }
        const auto& value = it->second;
        if (desc.kind == ParameterKind::Scalar || desc.kind == ParameterKind::Vector)
        {
            const std::size_t copyBytes = desc.kind == ParameterKind::Scalar ? sizeof(float) : sizeof(float) * 4;
            if (desc.uniformOffset + copyBytes <= snap.uniformBlock.size())
            {
                std::memcpy(snap.uniformBlock.data() + desc.uniformOffset, value.vec, copyBytes);
            }
        }
        else if (desc.kind == ParameterKind::Texture)
        {
            if (desc.textureSlot < snap.textureHandles.size())
            {
                snap.textureHandles[desc.textureSlot] = value.textureHandle;
            }
        }
    }
    return snap;
}

} // namespace Hylux::Renderer
