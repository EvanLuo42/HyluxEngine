/// @file
/// @brief EnginePaths implementation.

#include "Core/Paths/EnginePaths.h"

#include <system_error>

#if defined(HYLUX_WINDOWS)
    #include <windows.h>
    #include <shlobj.h>
    #include <knownfolders.h>
    #include <objbase.h>
#endif

namespace Hylux::EnginePaths
{
namespace
{

std::filesystem::path ExecutableDirectory()
{
#if defined(HYLUX_WINDOWS)
    wchar_t buffer[MAX_PATH * 4];
    const DWORD n = GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(std::size(buffer)));
    if (n == 0 || n == std::size(buffer)) return {};
    return std::filesystem::path(buffer, buffer + n).parent_path();
#else
    std::error_code ec;
    return std::filesystem::current_path(ec);
#endif
}

std::filesystem::path DiscoverRepoRoot()
{
    std::error_code ec;
    auto dir = ExecutableDirectory();
    if (dir.empty()) return {};

    for (int hops = 0; hops < 16; ++hops)
    {
        if (std::filesystem::exists(dir / "vcpkg.json", ec)) return dir;
        const auto parent = dir.parent_path();
        if (parent == dir) break;
        dir = parent;
    }
    return {};
}

std::filesystem::path DiscoverUserAppDataRoot()
{
#if defined(HYLUX_WINDOWS)
    PWSTR raw = nullptr;
    const HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &raw);
    if (FAILED(hr) || raw == nullptr)
    {
        if (raw) CoTaskMemFree(raw);
        return {};
    }
    std::filesystem::path out = std::filesystem::path(raw) / "HyluxEngine";
    CoTaskMemFree(raw);
    return out;
#else
    return {};
#endif
}

} // namespace

const std::filesystem::path& RepoRoot()
{
    static const std::filesystem::path kRoot = DiscoverRepoRoot();
    return kRoot;
}

const std::filesystem::path& ProjectContentRoot()
{
    static const std::filesystem::path kRoot = RepoRoot().empty()
                                                   ? std::filesystem::path{}
                                                   : RepoRoot() / "Content";
    return kRoot;
}

const std::filesystem::path& UserAppDataRoot()
{
    static const std::filesystem::path kRoot = DiscoverUserAppDataRoot();
    return kRoot;
}

} // namespace Hylux::EnginePaths
