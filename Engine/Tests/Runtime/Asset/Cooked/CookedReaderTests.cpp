/// @file
/// @brief Tests for CookedReader (header validation + accessors).

#include "Asset/Cooked/CookedReader.h"
#include "Core/Guid.h"
#include "Core/IO/IFile.h"
#include "Core/IO/Virtual/Providers/LooseFileProvider.h"
#include "Core/IO/Virtual/VirtualFileSystem.h"
#include "../CookedFixture.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Asset::Cooked");

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
    auto path = std::filesystem::temp_directory_path() / ("hylux_cooked_" + std::string(suffix) + "_" + stamp);
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

struct VfsFixture
{
    TempDirGuard guard;
    VirtualFileSystem vfs;
    explicit VfsFixture(const char* tag) : guard(tag)
    {
        vfs.Mount("/Engine/", std::make_shared<LooseFileProvider>(guard.path), 0);
    }
};

} // namespace

TEST_CASE("CookedReader::Open: missing file -> nullopt (no log noise)")
{
    VfsFixture f{"missing"};
    auto r = Asset::Cooked::CookedReader::Open(f.vfs, "/Engine/nope.hass");
    CHECK_FALSE(r.has_value());
}

TEST_CASE("CookedReader::Open: valid header + payload survives round-trip")
{
    VfsFixture f{"ok"};
    const auto guid = Guid::Generate();
    const auto bytes = BuildCookedFile(Asset::AssetTypeId::Material, guid, "PAYLOAD");
    Write(f.guard.path / "asset.hass", bytes);

    auto r = Asset::Cooked::CookedReader::Open(f.vfs, "/Engine/asset.hass");
    REQUIRE(r.has_value());
    CHECK(r->TypeTag() == Asset::AssetTypeId::Material);
    CHECK(r->GuidValue() == guid);
    auto payload = r->PayloadBytes();
    REQUIRE(payload.size() == 7u);
    CHECK(std::string(reinterpret_cast<const char*>(payload.data()), payload.size()) == "PAYLOAD");
    CHECK(r->RefCount() == 0u);
}

TEST_CASE("CookedReader::Open: rejects bad magic / endian / version")
{
    VfsFixture f{"bad_magic"};
    Write(f.guard.path / "a.hass", BuildCorruptCookedFile(HeaderCorruption::Magic));
    CHECK_FALSE(Asset::Cooked::CookedReader::Open(f.vfs, "/Engine/a.hass").has_value());

    Write(f.guard.path / "b.hass", BuildCorruptCookedFile(HeaderCorruption::Endian));
    CHECK_FALSE(Asset::Cooked::CookedReader::Open(f.vfs, "/Engine/b.hass").has_value());

    Write(f.guard.path / "c.hass", BuildCorruptCookedFile(HeaderCorruption::Version));
    CHECK_FALSE(Asset::Cooked::CookedReader::Open(f.vfs, "/Engine/c.hass").has_value());
}

TEST_CASE("CookedReader::Open: rejects payload / ref-table overruns")
{
    VfsFixture f{"overrun"};
    Write(f.guard.path / "p.hass", BuildCorruptCookedFile(HeaderCorruption::PayloadOverrun));
    CHECK_FALSE(Asset::Cooked::CookedReader::Open(f.vfs, "/Engine/p.hass").has_value());

    Write(f.guard.path / "r.hass", BuildCorruptCookedFile(HeaderCorruption::RefTableOverrun));
    CHECK_FALSE(Asset::Cooked::CookedReader::Open(f.vfs, "/Engine/r.hass").has_value());
}

TEST_CASE("CookedReader::Open: file smaller than HassHeader -> nullopt")
{
    VfsFixture f{"small"};
    Write(f.guard.path / "tiny.hass", std::vector<std::uint8_t>(16u, 0u));
    CHECK_FALSE(Asset::Cooked::CookedReader::Open(f.vfs, "/Engine/tiny.hass").has_value());
}

TEST_CASE("CookedReader::Ref: index out of range -> empty SoftAssetRef")
{
    VfsFixture f{"oob_ref"};
    Write(f.guard.path / "a.hass",
          BuildCookedFile(Asset::AssetTypeId::Mesh, Guid::Generate(), "p"));
    auto r = Asset::Cooked::CookedReader::Open(f.vfs, "/Engine/a.hass");
    REQUIRE(r.has_value());
    CHECK(r->RefCount() == 0u);
    const auto ref = r->Ref(0);
    CHECK_FALSE(ref.IsValid());
}

TEST_CASE("CookedReader::Ref: index in range returns guid + path-hint")
{
    VfsFixture f{"ref"};
    const auto ownerGuid = Guid::Generate();
    const auto childGuid = Guid::Generate();
    std::vector<RefDesc> refs{
        RefDesc{childGuid, "/Game/child.hass"},
        RefDesc{Guid::Generate(), ""},
    };
    Write(f.guard.path / "owner.hass",
          BuildCookedFile(Asset::AssetTypeId::MaterialInstance, ownerGuid, "p", refs));

    auto r = Asset::Cooked::CookedReader::Open(f.vfs, "/Engine/owner.hass");
    REQUIRE(r.has_value());
    CHECK(r->RefCount() == 2u);
    auto first = r->Ref(0);
    CHECK(first.IsValid());
    CHECK(first.guid == childGuid);
    CHECK(first.pathHint == "/Game/child.hass");

    auto second = r->Ref(1);
    CHECK(second.IsValid());
    CHECK(second.pathHint.empty());
}

TEST_CASE("CookedReader: At / Range / ByteRange honor in-bounds checks")
{
    VfsFixture f{"ranges"};
    const auto bytes = BuildCookedFile(Asset::AssetTypeId::Mesh, Guid::Generate(),
                                       std::string_view{"ABCDEFGH", 8});
    Write(f.guard.path / "x.hass", bytes);
    auto r = Asset::Cooked::CookedReader::Open(f.vfs, "/Engine/x.hass");
    REQUIRE(r.has_value());

    const auto* fourBytes = r->At<std::uint32_t>(static_cast<std::uint32_t>(sizeof(Asset::Cooked::HassHeader)));
    REQUIRE(fourBytes != nullptr);

    // Out-of-bounds offset returns nullptr / empty.
    CHECK(r->At<std::uint32_t>(0xFFFF0000u) == nullptr);
    CHECK(r->Range<std::uint32_t>(0xFFFF0000u, 16).empty());
    CHECK(r->ByteRange(0xFFFF0000u, 16).empty());

    auto payload = r->ByteRange(static_cast<std::uint32_t>(sizeof(Asset::Cooked::HassHeader)), 4);
    REQUIRE(payload.size() == 4u);
    CHECK(static_cast<char>(payload[0]) == 'A');
}

TEST_SUITE_END();
