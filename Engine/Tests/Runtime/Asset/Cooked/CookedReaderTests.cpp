/// @file
/// @brief Tests for CookedReader. We hand-build a minimal valid .hass byte stream, write
///        it through a temp-dir LooseFileProvider mounted at /Test/, and confirm the
///        reader exposes header, payload, and ref-table data correctly.

#include "Asset/Cooked/CookedFormat.h"
#include "Asset/Cooked/CookedHeader.h"
#include "Asset/Cooked/CookedReader.h"
#include "Asset/Guid.h"
#include "Core/IO/IFile.h"
#include "Core/IO/Virtual/Providers/LooseFileProvider.h"
#include "Core/IO/Virtual/VirtualFileSystem.h"

#include <doctest/doctest.h>

#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

using namespace Hylux;
using namespace Hylux::Asset;
using namespace Hylux::Asset::Cooked;

namespace
{

std::filesystem::path MakeTempDir(const char* tag)
{
    const auto base  = std::filesystem::temp_directory_path();
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    auto       dir   = base / (std::string{"hylux-hass-test-"} + tag + "-" + std::to_string(stamp));
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

void RemoveAllSilent(const std::filesystem::path& p)
{
    std::error_code ec;
    std::filesystem::remove_all(p, ec);
}

void WriteFile(const std::filesystem::path& path, const std::vector<std::byte>& bytes)
{
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

template<typename T>
void AppendPod(std::vector<std::byte>& buf, const T& value)
{
    const auto* p = reinterpret_cast<const std::byte*>(&value);
    buf.insert(buf.end(), p, p + sizeof(T));
}

void AppendBytes(std::vector<std::byte>& buf, std::string_view bytes)
{
    for (char c : bytes)
    {
        buf.push_back(static_cast<std::byte>(c));
    }
}

void PadTo(std::vector<std::byte>& buf, std::size_t target)
{
    while (buf.size() < target)
    {
        buf.push_back(std::byte{0});
    }
}

/// @brief Builds a minimal valid Material .hass with `refCount` cross-asset refs (each
///        with a sample path) and a 4-parameter MaterialPayload after the ref table.
std::vector<std::byte> BuildMaterialHass(Guid assetGuid, std::uint32_t refCount)
{
    std::vector<std::byte> buf;

    HassHeader header{};
    header.magic     = kHassMagic;
    header.endianTag = kHassEndianTag;
    header.version   = kHassVersion;
    header.typeTag   = static_cast<std::uint16_t>(AssetTypeId::Material);
    std::memcpy(header.guid, assetGuid.bytes.data(), Guid::kSize);
    AppendPod(buf, header);
    REQUIRE(buf.size() == kHassHeaderSize);

    const std::uint32_t refTableOffset = kHassHeaderSize;
    std::uint32_t       runningOffset  = 0;
    std::vector<std::string> paths;
    paths.reserve(refCount);
    for (std::uint32_t i = 0; i < refCount; ++i)
    {
        paths.emplace_back("/Game/Textures/Ref" + std::to_string(i) + ".htex");
    }

    for (std::uint32_t i = 0; i < refCount; ++i)
    {
        RefEntry e{};
        e.guid[0]    = static_cast<std::uint8_t>(0xA0u + i);
        e.guid[15]   = static_cast<std::uint8_t>(0x10u + i);
        e.pathOffset = runningOffset;
        e.pathLength = static_cast<std::uint32_t>(paths[i].size());
        runningOffset += e.pathLength;
        AppendPod(buf, e);
    }

    for (const auto& p : paths)
    {
        AppendBytes(buf, p);
    }

    while (buf.size() % 16 != 0)
    {
        buf.push_back(std::byte{0});
    }

    const std::uint32_t payloadOffset = static_cast<std::uint32_t>(buf.size());
    MaterialPayload     payload{};
    payload.shaderMapHash      = 0xDEADBEEFCAFEBABEull;
    payload.permutationBaseKey = 0x0000000000000007ull;
    payload.uniformBlockSize   = 64;
    payload.textureCount       = refCount;
    payload.parameterOffset    = payloadOffset + static_cast<std::uint32_t>(sizeof(MaterialPayload));
    payload.parameterCount     = 0;
    AppendPod(buf, payload);

    auto& mutableHeader        = *reinterpret_cast<HassHeader*>(buf.data());
    mutableHeader.payloadOffset = payloadOffset;
    mutableHeader.payloadSize   = static_cast<std::uint32_t>(sizeof(MaterialPayload));
    mutableHeader.refTableOffset = refTableOffset;
    mutableHeader.refTableCount  = refCount;

    PadTo(buf, ((buf.size() + 15) / 16) * 16);
    return buf;
}

} // namespace

TEST_CASE("CookedReader opens a hand-built Material .hass and exposes header + payload")
{
    const auto tempDir = MakeTempDir("hass-material");

    const Guid assetGuid =
        *Guid::Parse("550e8400-e29b-41d4-a716-446655440099");
    const auto bytes = BuildMaterialHass(assetGuid, /*refCount*/ 2);
    WriteFile(tempDir / "Test.hass", bytes);

    VirtualFileSystem vfs;
    vfs.Mount("/Test/", std::make_shared<LooseFileProvider>(tempDir), 0);

    auto reader = CookedReader::Open(vfs, "/Test/Test.hass");
    REQUIRE(reader.has_value());

    CHECK(reader->TypeTag() == AssetTypeId::Material);
    CHECK(reader->GuidValue() == assetGuid);
    CHECK(reader->RefCount() == 2u);

    const auto* payload = reader->PayloadAs<MaterialPayload>();
    REQUIRE(payload != nullptr);
    CHECK(payload->shaderMapHash == 0xDEADBEEFCAFEBABEull);
    CHECK(payload->permutationBaseKey == 0x0000000000000007ull);
    CHECK(payload->uniformBlockSize == 64u);
    CHECK(payload->textureCount == 2u);

    SoftAssetRef ref0 = reader->Ref(0);
    CHECK(ref0.IsValid());
    CHECK(ref0.guid.bytes[0] == 0xA0u);
    CHECK(ref0.pathHint == "/Game/Textures/Ref0.htex");

    SoftAssetRef ref1 = reader->Ref(1);
    CHECK(ref1.IsValid());
    CHECK(ref1.pathHint == "/Game/Textures/Ref1.htex");

    SoftAssetRef refOob = reader->Ref(5);
    CHECK_FALSE(refOob.IsValid());

    RemoveAllSilent(tempDir);
}

TEST_CASE("CookedReader rejects bad magic")
{
    const auto tempDir = MakeTempDir("hass-badmagic");
    auto       bytes   = BuildMaterialHass(Guid::Generate(), 0);
    auto&      header  = *reinterpret_cast<HassHeader*>(bytes.data());
    header.magic       = 0xBADBADBAu;
    WriteFile(tempDir / "Bad.hass", bytes);

    VirtualFileSystem vfs;
    vfs.Mount("/Test/", std::make_shared<LooseFileProvider>(tempDir), 0);

    CHECK_FALSE(CookedReader::Open(vfs, "/Test/Bad.hass").has_value());

    RemoveAllSilent(tempDir);
}

TEST_CASE("CookedReader rejects unsupported version")
{
    const auto tempDir = MakeTempDir("hass-badver");
    auto       bytes   = BuildMaterialHass(Guid::Generate(), 0);
    auto&      header  = *reinterpret_cast<HassHeader*>(bytes.data());
    header.version     = 99;
    WriteFile(tempDir / "Bad.hass", bytes);

    VirtualFileSystem vfs;
    vfs.Mount("/Test/", std::make_shared<LooseFileProvider>(tempDir), 0);

    CHECK_FALSE(CookedReader::Open(vfs, "/Test/Bad.hass").has_value());

    RemoveAllSilent(tempDir);
}

TEST_CASE("CookedReader rejects truncated payload offset")
{
    const auto tempDir = MakeTempDir("hass-trunc");
    auto       bytes   = BuildMaterialHass(Guid::Generate(), 0);
    auto&      header  = *reinterpret_cast<HassHeader*>(bytes.data());
    header.payloadSize = 0xFFFFFFFFu;
    WriteFile(tempDir / "Bad.hass", bytes);

    VirtualFileSystem vfs;
    vfs.Mount("/Test/", std::make_shared<LooseFileProvider>(tempDir), 0);

    CHECK_FALSE(CookedReader::Open(vfs, "/Test/Bad.hass").has_value());

    RemoveAllSilent(tempDir);
}

TEST_CASE("CookedReader returns null on missing file")
{
    const auto        tempDir = MakeTempDir("hass-missing");
    VirtualFileSystem vfs;
    vfs.Mount("/Test/", std::make_shared<LooseFileProvider>(tempDir), 0);

    CHECK_FALSE(CookedReader::Open(vfs, "/Test/Nope.hass").has_value());

    RemoveAllSilent(tempDir);
}
