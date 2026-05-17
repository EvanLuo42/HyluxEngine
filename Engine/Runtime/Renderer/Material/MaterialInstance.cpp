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

MaterialInstance::MaterialInstance(const Asset::MaterialAsset* asset) noexcept : asset_(asset) {}

std::uint64_t MaterialInstance::GetAssetHash() const noexcept
{
    return asset_ != nullptr ? asset_->GetShaderMapHash() : 0ull;
}

void MaterialInstance::MarkDirty() const noexcept
{
    hashDirty_ = true;
}

void MaterialInstance::SetScalar(Asset::NameHash name, float value)
{
    if (asset_ == nullptr)
    {
        return;
    }
    const auto* desc = asset_->FindParameter(name);
    if (desc == nullptr || desc->kind != Asset::ParameterKind::Scalar)
    {
        return;
    }
    Asset::ParameterValue& slot = overrides_[name];
    slot.kind   = Asset::ParameterKind::Scalar;
    slot.vec[0] = value;
    MarkDirty();
}

void MaterialInstance::SetVector(Asset::NameHash name, float x, float y, float z, float w)
{
    if (asset_ == nullptr)
    {
        return;
    }
    const auto* desc = asset_->FindParameter(name);
    if (desc == nullptr || desc->kind != Asset::ParameterKind::Vector)
    {
        return;
    }
    Asset::ParameterValue& slot = overrides_[name];
    slot.kind   = Asset::ParameterKind::Vector;
    slot.vec[0] = x;
    slot.vec[1] = y;
    slot.vec[2] = z;
    slot.vec[3] = w;
    MarkDirty();
}

void MaterialInstance::SetTexture(Asset::NameHash name, Ref<Asset::TextureAsset> texture)
{
    if (asset_ == nullptr)
    {
        return;
    }
    const auto* desc = asset_->FindParameter(name);
    if (desc == nullptr || desc->kind != Asset::ParameterKind::Texture)
    {
        return;
    }
    Asset::ParameterValue& slot = overrides_[name];
    slot.kind                 = Asset::ParameterKind::Texture;
    slot.textureBindlessIndex = texture ? texture->GetBindlessIndex()
                                        : Asset::ParameterValue::kInvalidBindlessIndex;
    if (texture)
    {
        textureRefs_[name] = std::move(texture);
    }
    else
    {
        textureRefs_.erase(name);
    }
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
        const auto assetHash = asset_->GetShaderMapHash();
        h = HashMix(h, &assetHash, sizeof(assetHash));
    }
    for (const auto& [name, value] : overrides_)
    {
        const auto nameRaw = Asset::ToU64(name);
        h = HashMix(h, &nameRaw, sizeof(nameRaw));
        h = HashMix(h, &value, sizeof(value));
    }
    cachedInstanceHash_ = h;

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
            const auto nameRaw = Asset::ToU64(desc.name);
            perm = HashMix(perm, &nameRaw, sizeof(nameRaw));
            perm = HashMix(perm, &it->second, sizeof(it->second));
        }
    }
    cachedPermutationKey_ = perm;
    hashDirty_            = false;
    return cachedInstanceHash_;
}

std::uint64_t MaterialInstance::GetPermutationKey() const noexcept
{
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
    snap.materialAssetHash = asset_->GetShaderMapHash();
    snap.instanceHash      = GetInstanceHash();
    snap.permutationKey    = cachedPermutationKey_;

    snap.uniformBlock.assign(asset_->GetUniformBlockSize(), std::byte{0});
    snap.textureBindlessIndices.assign(asset_->GetTextureCount(),
                                       Asset::ParameterValue::kInvalidBindlessIndex);
    snap.referencedTextures.reserve(textureRefs_.size());

    for (const auto& desc : asset_->GetParameters())
    {
        const auto it = overrides_.find(desc.name);
        if (it == overrides_.end())
        {
            continue;
        }
        const auto& value = it->second;
        if (desc.kind == Asset::ParameterKind::Scalar || desc.kind == Asset::ParameterKind::Vector)
        {
            const std::size_t copyBytes =
                desc.kind == Asset::ParameterKind::Scalar ? sizeof(float) : sizeof(float) * 4;
            if (desc.uniformOffset + copyBytes <= snap.uniformBlock.size())
            {
                std::memcpy(snap.uniformBlock.data() + desc.uniformOffset, value.vec, copyBytes);
            }
        }
        else if (desc.kind == Asset::ParameterKind::Texture)
        {
            if (desc.textureSlot < snap.textureBindlessIndices.size())
            {
                snap.textureBindlessIndices[desc.textureSlot] = value.textureBindlessIndex;
            }
            const auto refIt = textureRefs_.find(desc.name);
            if (refIt != textureRefs_.end())
            {
                snap.referencedTextures.push_back(refIt->second);
            }
        }
    }
    return snap;
}

} // namespace Hylux::Renderer
