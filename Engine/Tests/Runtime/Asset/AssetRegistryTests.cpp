/// @file
/// @brief Tests for AssetRegistry (Scan / Find / FindByPath / Clear / TypeCounts).

#include "Asset/AssetRegistry.h"
#include "Core/Guid.h"
#include "Core/IO/Virtual/Providers/LooseFileProvider.h"
#include "Core/IO/Virtual/VirtualFileSystem.h"
#include "CookedFixture.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Asset");

#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <string>

using namespace Hylux;
using namespace Hylux::Tests;

namespace
{

std::filesystem::path UniqueTempDir(const char* suffix)
{
    std::random_device rd;
    const auto stamp = std::to_string(rd()) + "_" + std::to_string(rd());
    auto path = std::filesystem::temp_directory_path() / ("hylux_areg_" + std::string(suffix) + "_" + stamp);
    std::filesystem::create_directories(path);
    return path;
}

struct TempDirGuard
{
    std::filesystem::path path;
    explicit TempDirGuard(const char* tag) : path(UniqueTempDir(tag)) {}
    ~TempDirGuard()
    {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

void Write(const std::filesystem::path& p, const std::vector<std::uint8_t>& bytes)
{
    std::filesystem::create_directories(p.parent_path());
    std::ofstream os(p, std::ios::binary | std::ios::trunc);
    os.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void WriteAsset(const std::filesystem::path& dir, std::string_view rel,
                Asset::AssetTypeId type, Guid guid)
{
    Write(dir / rel, BuildCookedFile(type, guid, "p"));
}

} // namespace

TEST_CASE("AssetRegistry::Scan: empty mount yields zero entries; Size/Empty/Find")
{
    TempDirGuard guard{"empty"};
    VirtualFileSystem vfs;
    vfs.Mount("/Engine/", std::make_shared<LooseFileProvider>(guard.path), 0);
    Asset::AssetRegistry registry;
    CHECK(registry.Scan(vfs, "/Engine/") == 0u);
    CHECK(registry.Size() == 0u);
    CHECK_FALSE(registry.Find(Guid::Generate()).has_value());
    CHECK_FALSE(registry.FindByPath("/Engine/missing.hass").has_value());
}

TEST_CASE("AssetRegistry::Scan: indexes all .hass files; ComputeTypeCounts buckets correctly")
{
    TempDirGuard guard{"scan"};
    const auto meshGuid    = Guid::Generate();
    const auto matGuid     = Guid::Generate();
    const auto texGuid     = Guid::Generate();
    const auto miGuid      = Guid::Generate();
    WriteAsset(guard.path, "mesh.hass",     Asset::AssetTypeId::Mesh,             meshGuid);
    WriteAsset(guard.path, "sub/mat.hass",  Asset::AssetTypeId::Material,         matGuid);
    WriteAsset(guard.path, "tex.hass",      Asset::AssetTypeId::Texture,          texGuid);
    WriteAsset(guard.path, "sub/mi.hass",   Asset::AssetTypeId::MaterialInstance, miGuid);
    // Non-.hass file is ignored.
    {
        std::ofstream{guard.path / "readme.txt", std::ios::binary} << "ignore";
    }

    VirtualFileSystem vfs;
    vfs.Mount("/Engine/", std::make_shared<LooseFileProvider>(guard.path), 0);
    Asset::AssetRegistry registry;
    CHECK(registry.Scan(vfs, "/Engine/") == 4u);
    CHECK(registry.Size() == 4u);

    CHECK(registry.Find(meshGuid).has_value());
    CHECK(registry.Find(matGuid)->type == Asset::AssetTypeId::Material);
    CHECK(registry.Find(texGuid)->type == Asset::AssetTypeId::Texture);
    CHECK(registry.Find(miGuid)->type == Asset::AssetTypeId::MaterialInstance);

    CHECK(registry.FindByPath("/Engine/mesh.hass").has_value());
    CHECK(registry.FindByPath("/Engine/sub/mat.hass")->guid == matGuid);
    CHECK_FALSE(registry.FindByPath("/Engine/no.hass").has_value());

    const auto counts = registry.ComputeTypeCounts();
    CHECK(counts.mesh == 1u);
    CHECK(counts.material == 1u);
    CHECK(counts.materialInstance == 1u);
    CHECK(counts.texture == 1u);
    CHECK(counts.unknown == 0u);
}

TEST_CASE("AssetRegistry::Scan: skips corrupt files (header validation fails)")
{
    TempDirGuard guard{"corrupt"};
    Write(guard.path / "tiny.hass", std::vector<std::uint8_t>(8u, 0u));
    Write(guard.path / "bad_magic.hass", BuildCorruptCookedFile(HeaderCorruption::Magic));
    const auto good = Guid::Generate();
    WriteAsset(guard.path, "ok.hass", Asset::AssetTypeId::Mesh, good);

    VirtualFileSystem vfs;
    vfs.Mount("/Engine/", std::make_shared<LooseFileProvider>(guard.path), 0);
    Asset::AssetRegistry registry;
    CHECK(registry.Scan(vfs, "/Engine/") == 1u);
    CHECK(registry.Find(good).has_value());
}

TEST_CASE("AssetRegistry::Scan: duplicate guid logs warn and keeps first")
{
    TempDirGuard guard{"dup"};
    const auto g = Guid::Generate();
    WriteAsset(guard.path, "a.hass", Asset::AssetTypeId::Mesh, g);
    WriteAsset(guard.path, "b.hass", Asset::AssetTypeId::Texture, g);

    VirtualFileSystem vfs;
    vfs.Mount("/Engine/", std::make_shared<LooseFileProvider>(guard.path), 0);
    Asset::AssetRegistry registry;
    CHECK(registry.Scan(vfs, "/Engine/") == 1u);
    // First-seen wins. EnumerateFiles order is unspecified, so the test only verifies
    // that exactly one entry exists for the guid.
    CHECK(registry.Find(g).has_value());
}

TEST_CASE("AssetRegistry::Clear: empties entries and pathToGuid maps")
{
    TempDirGuard guard{"clear"};
    WriteAsset(guard.path, "x.hass", Asset::AssetTypeId::Mesh, Guid::Generate());
    VirtualFileSystem vfs;
    vfs.Mount("/Engine/", std::make_shared<LooseFileProvider>(guard.path), 0);
    Asset::AssetRegistry registry;
    registry.Scan(vfs, "/Engine/");
    CHECK(registry.Size() == 1u);
    registry.Clear();
    CHECK(registry.Size() == 0u);
    CHECK_FALSE(registry.FindByPath("/Engine/x.hass").has_value());
}

TEST_SUITE_END();
