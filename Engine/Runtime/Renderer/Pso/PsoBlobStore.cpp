/// @file
/// @brief PsoBlobStore implementation.

#include "Renderer/Pso/PsoBlobStore.h"

#include "Core/Logging/CoreLogCategories.h"
#include "Core/Logging/Logger.h"

#include <cstdio>
#include <fstream>
#include <system_error>

namespace Hylux::Renderer
{

PsoBlobStore::PsoBlobStore(std::filesystem::path directory) : directory_(std::move(directory))
{
    if (directory_.empty())
    {
        return;
    }
    std::error_code ec;
    std::filesystem::create_directories(directory_, ec);
    if (ec)
    {
        HYLUX_LOG_WARN(LogRender, "PsoBlobStore: failed to create '{}': {}",
                       directory_.string(), ec.message());
    }
}

std::filesystem::path PsoBlobStore::BuildPath(std::string_view name) const
{
    return directory_ / (std::string(name) + ".psoblob");
}

std::vector<std::byte> PsoBlobStore::Load(std::string_view name) const
{
    if (directory_.empty())
    {
        return {};
    }
    const auto path = BuildPath(name);
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec)
    {
        return {};
    }
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
    {
        return {};
    }
    const auto size = static_cast<std::streamoff>(stream.tellg());
    if (size <= 0)
    {
        return {};
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::byte> blob(static_cast<std::size_t>(size));
    stream.read(reinterpret_cast<char*>(blob.data()), size);
    if (!stream)
    {
        return {};
    }
    return blob;
}

bool PsoBlobStore::Save(std::string_view name, std::span<const std::byte> blob)
{
    if (directory_.empty())
    {
        return false;
    }
    const auto finalPath = BuildPath(name);
    auto       tempPath  = finalPath;
    tempPath += ".tmp";

    {
        std::ofstream stream(tempPath, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            HYLUX_LOG_WARN(LogRender, "PsoBlobStore: open for write failed at '{}'",
                           tempPath.string());
            return false;
        }
        if (!blob.empty())
        {
            stream.write(reinterpret_cast<const char*>(blob.data()),
                         static_cast<std::streamsize>(blob.size()));
        }
        if (!stream)
        {
            HYLUX_LOG_WARN(LogRender, "PsoBlobStore: write failed at '{}'", tempPath.string());
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tempPath, finalPath, ec);
    if (ec)
    {
        // Fallback: remove the destination then retry (Windows rename can fail on overwrite).
        std::filesystem::remove(finalPath, ec);
        std::filesystem::rename(tempPath, finalPath, ec);
        if (ec)
        {
            HYLUX_LOG_WARN(LogRender, "PsoBlobStore: rename failed '{}' -> '{}': {}",
                           tempPath.string(), finalPath.string(), ec.message());
            std::filesystem::remove(tempPath, ec);
            return false;
        }
    }
    return true;
}

} // namespace Hylux::Renderer
