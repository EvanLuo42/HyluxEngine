/// @file
/// @brief ShaderModuleCache implementation. Translates a resolved ShaderRecord into a
///        Ref<IRHIShaderModule> via IRHIDevice::CreateShaderModule on first request.

#include "Shader/ShaderMap/ShaderModuleCache.h"

#include "Shader/Diagnostics/ShaderLogCategories.h"

#include "Core/Logging/Logger.h"
#include "RHI/IRHIDevice.h"
#include "RHI/IRHIShaderModule.h"

namespace Hylux::Shader
{

ShaderModuleCache::ShaderModuleCache(RHI::IRHIDevice* device, const IShaderArchive* archive) noexcept
    : device_(device), archive_(archive)
{
}

Ref<RHI::IRHIShaderModule> ShaderModuleCache::GetOrCreate(const ShaderRecord& record)
{
    if (auto it = modules_.find(record.id.value); it != modules_.end())
    {
        return it->second;
    }

    if (device_ == nullptr || record.bytecode.empty())
    {
        HYLUX_LOG_ERROR(LogShader,
                        "ShaderModuleCache::GetOrCreate: cannot materialize id 0x{:016x} "
                        "(device={}, bytecode={} bytes)",
                        record.id.value, static_cast<const void*>(device_),
                        record.bytecode.size());
        return Ref<RHI::IRHIShaderModule>{};
    }

    RHI::ShaderBytecode bytecode{};
    bytecode.format     = record.format;
    bytecode.stage      = record.stage;
    bytecode.entryPoint = record.entryPoint;
    bytecode.data       = record.bytecode;

    auto module = device_->CreateShaderModule(bytecode);
    if (!module)
    {
        HYLUX_LOG_ERROR(LogShader,
                        "ShaderModuleCache::GetOrCreate: RHI rejected id 0x{:016x} "
                        "(stage={}, bytes={})",
                        record.id.value, static_cast<std::uint32_t>(record.stage),
                        record.bytecode.size());
        return Ref<RHI::IRHIShaderModule>{};
    }

    modules_.emplace(record.id.value, module);
    return module;
}

void ShaderModuleCache::InvalidateAll() noexcept
{
    modules_.clear();
}

void ShaderModuleCache::Prune(const IShaderArchive& archive)
{
    for (auto it = modules_.begin(); it != modules_.end(); )
    {
        if (archive.ContainsId(ShaderId{it->first}))
        {
            ++it;
        }
        else
        {
            it = modules_.erase(it);
        }
    }
}

} // namespace Hylux::Shader
