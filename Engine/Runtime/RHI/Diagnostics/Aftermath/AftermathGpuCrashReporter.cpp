/// @file
/// @brief AftermathGpuCrashReporter implementation.

#include "RHI/Diagnostics/Aftermath/AftermathGpuCrashReporter.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"
#include "RHI/IRHIObject.h"
#include "RHI/Vulkan/VulkanCommandList.h"

#include <GFSDK_Aftermath.h>
#include <GFSDK_Aftermath_GpuCrashDumpDecoding.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>

namespace Hylux::RHI::Vulkan
{

namespace
{

std::string TimestampedFileName(std::string_view prefix, std::string_view suffix)
{
    const auto now    = std::chrono::system_clock::now();
    const auto epochS = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    std::string out;
    out.reserve(prefix.size() + 24 + suffix.size());
    out.append(prefix);
    out.append("-");
    out.append(std::to_string(static_cast<std::uint64_t>(epochS)));
    out.append(suffix);
    return out;
}

std::string DefaultDumpDirectory()
{
    namespace fs = std::filesystem;
    fs::path p = fs::current_path() / "AftermathDumps";
    std::error_code ec;
    fs::create_directories(p, ec);
    return p.string();
}

void WriteBlob(const std::string& path, const void* data, std::size_t size)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return;
    f.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
}

} // namespace

AftermathGpuCrashReporter::AftermathGpuCrashReporter()
{
    dumpDirectory_ = DefaultDumpDirectory();

    const GFSDK_Aftermath_Result r = GFSDK_Aftermath_EnableGpuCrashDumps(
        GFSDK_Aftermath_Version_API,
        GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
        GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
        &AftermathGpuCrashReporter::CrashDumpCb,
        &AftermathGpuCrashReporter::ShaderDebugInfoCb,
        &AftermathGpuCrashReporter::DescriptionCb,
        &AftermathGpuCrashReporter::ResolveMarkerCb,
        this);

    if (GFSDK_Aftermath_SUCCEED(r))
    {
        enabled_ = true;
        HYLUX_LOG(::Hylux::LogRender, Info,
                  "Aftermath: GpuCrashDumps enabled; dump directory = {}", dumpDirectory_);
    }
    else
    {
        HYLUX_LOG(::Hylux::LogRender, Error,
                  "Aftermath: GFSDK_Aftermath_EnableGpuCrashDumps failed (result={})",
                  static_cast<int>(r));
    }
}

AftermathGpuCrashReporter::~AftermathGpuCrashReporter()
{
    if (enabled_)
    {
        GFSDK_Aftermath_DisableGpuCrashDumps();
    }
}

void AftermathGpuCrashReporter::BindDevice(VkDevice device) noexcept
{
    device_ = device;
}

void AftermathGpuCrashReporter::RegisterShader(const ShaderBinaryRegistration& reg)
{
    if (!enabled_) return;
    AftermathShaderEntry entry;
    entry.shaderHash = reg.shaderHash;
    entry.format     = reg.format;
    entry.stage      = reg.stage;
    entry.debugName.assign(reg.debugName);
    if (reg.bytecode && reg.bytecodeSize > 0)
    {
        entry.bytecode.resize(reg.bytecodeSize);
        std::memcpy(entry.bytecode.data(), reg.bytecode, reg.bytecodeSize);
    }
    if (reg.sourceMappingData && reg.sourceMappingSize > 0)
    {
        entry.sourceMapping.resize(reg.sourceMappingSize);
        std::memcpy(entry.sourceMapping.data(), reg.sourceMappingData, reg.sourceMappingSize);
    }
    shaderDb_.Register(entry);
}

void AftermathGpuCrashReporter::RegisterResource(const ResourceRegistration& /*reg*/)
{
    // Aftermath on Vulkan auto-tracks resources through the diagnostics extension;
    // explicit registration is unnecessary. Engine code may still call this — no-op.
}

void AftermathGpuCrashReporter::UnregisterResource(IRHIObject* /*resource*/)
{
    // See RegisterResource — auto-tracked, nothing to do.
}

const void* AftermathGpuCrashReporter::PinMarker(std::string_view name)
{
    std::lock_guard<std::mutex> lock(markerMutex_);
    if (markerRing_.size() >= markerRingCap_)
    {
        markerRing_.pop_front();
    }
    markerRing_.emplace_back(name);
    return markerRing_.back().c_str();
}

void AftermathGpuCrashReporter::PushMarker(IRHICommandList* commandList, std::string_view name)
{
    if (!enabled_ || !commandList || !vkCmdSetCheckpointNV) return;
    auto* vkCl = static_cast<VulkanCommandList*>(commandList);
    vkCmdSetCheckpointNV(vkCl->GetVkCommandBuffer(), PinMarker(name));
}

void AftermathGpuCrashReporter::PopMarker(IRHICommandList* /*commandList*/)
{
    // vkCmdSetCheckpointNV has no scope; the closing event is implicit at next checkpoint.
}

void AftermathGpuCrashReporter::InsertMarker(IRHICommandList* commandList, std::string_view name)
{
    PushMarker(commandList, name);
}

void AftermathGpuCrashReporter::SetCrashDumpCallback(GpuCrashDumpCallback cb, void* ud)
{
    userCallback_     = cb;
    userCallbackData_ = ud;
}

void AftermathGpuCrashReporter::SetCrashDumpDirectory(std::string_view directory)
{
    if (directory.empty())
    {
        dumpDirectory_ = DefaultDumpDirectory();
    }
    else
    {
        dumpDirectory_.assign(directory);
        std::error_code ec;
        std::filesystem::create_directories(dumpDirectory_, ec);
    }
}

GpuCrashStatus AftermathGpuCrashReporter::PollCrashStatus()
{
    if (!enabled_) return GpuCrashStatus::Unavailable;
    GFSDK_Aftermath_CrashDump_Status amStatus = GFSDK_Aftermath_CrashDump_Status_Unknown;
    GFSDK_Aftermath_GetCrashDumpStatus(&amStatus);
    switch (amStatus)
    {
        case GFSDK_Aftermath_CrashDump_Status_NotStarted:
            return GpuCrashStatus::NotCrashed;
        case GFSDK_Aftermath_CrashDump_Status_CollectingData:
        case GFSDK_Aftermath_CrashDump_Status_InvokingCallback:
            status_.store(GpuCrashStatus::Collecting, std::memory_order_release);
            return GpuCrashStatus::Collecting;
        case GFSDK_Aftermath_CrashDump_Status_Finished:
            return GpuCrashStatus::DumpReady;
        case GFSDK_Aftermath_CrashDump_Status_CollectingDataFailed:
            return GpuCrashStatus::Timeout;
        case GFSDK_Aftermath_CrashDump_Status_Unknown:
        default:
            return status_.load(std::memory_order_acquire);
    }
}

bool AftermathGpuCrashReporter::GetFaultInfo(GpuFaultInfo& outInfo) const
{
    std::lock_guard<std::mutex> lock(lastDumpMutex_);
    if (lastDumpPath_.empty()) return false;
    outInfo = lastFault_;
    return true;
}

// ----- static callbacks ------------------------------------------------------------------

void AftermathGpuCrashReporter::CrashDumpCb(const void* dumpData, std::uint32_t dumpSize, void* ud)
{
    auto* self = static_cast<AftermathGpuCrashReporter*>(ud);
    if (!self || !dumpData || dumpSize == 0) return;

    std::string filename = TimestampedFileName("crash", ".nv-gpudmp");
    std::string fullPath = self->dumpDirectory_ + "/" + filename;
    WriteBlob(fullPath, dumpData, dumpSize);

    // Decode minimal fault info (best-effort; ignore decoder errors).
    GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
    if (GFSDK_Aftermath_SUCCEED(
            GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
                GFSDK_Aftermath_Version_API, dumpData, dumpSize, &decoder)))
    {
        GFSDK_Aftermath_GpuCrashDump_BaseInfo base{};
        if (GFSDK_Aftermath_SUCCEED(
                GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &base)))
        {
            std::lock_guard<std::mutex> lock(self->lastDumpMutex_);
            self->lastFault_.kind = GpuFaultKind::Hang;
            self->lastFault_.message.assign("Aftermath dump captured");
            // Marker/VA decoding would go here via the page-fault info APIs.
        }
        GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder);
    }

    {
        std::lock_guard<std::mutex> lock(self->lastDumpMutex_);
        self->lastDumpPath_ = std::move(fullPath);
    }
    self->status_.store(GpuCrashStatus::DumpReady, std::memory_order_release);

    HYLUX_LOG(::Hylux::LogRender, Error,
              "Aftermath: GPU crash dump written to {} ({} bytes)",
              self->lastDumpPath_, dumpSize);

    if (self->userCallback_)
    {
        self->userCallback_(self->lastDumpPath_, self->userCallbackData_);
    }
}

void AftermathGpuCrashReporter::ShaderDebugInfoCb(const void* debugInfo, std::uint32_t size,
                                                  void* ud)
{
    auto* self = static_cast<AftermathGpuCrashReporter*>(ud);
    if (!self || !debugInfo || size == 0) return;
    std::string filename = TimestampedFileName("shader", ".nvdbg");
    WriteBlob(self->dumpDirectory_ + "/" + filename, debugInfo, size);
}

void AftermathGpuCrashReporter::DescriptionCb(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addValue,
                                              void* /*ud*/)
{
    addValue(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, "HyluxEngine");
    addValue(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "0.1.0");
}

void AftermathGpuCrashReporter::ResolveMarkerCb(
    const void* pMarkerData, std::uint32_t /*markerDataSize*/,
    void* /*pUserData*/, PFN_GFSDK_Aftermath_ResolveMarker resolveMarker)
{
    // PushMarker pins each label as a NUL-terminated string in the per-reporter ring;
    // hand the pointer + length back to Aftermath for inclusion in the crash dump.
    if (pMarkerData == nullptr || !resolveMarker) return;
    const auto len = static_cast<std::uint32_t>(std::strlen(static_cast<const char*>(pMarkerData)));
    resolveMarker(pMarkerData, len);
}

} // namespace Hylux::RHI::Vulkan
