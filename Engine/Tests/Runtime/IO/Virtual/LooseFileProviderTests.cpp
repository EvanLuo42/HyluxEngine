/// @file
/// @brief Unit tests for LooseFileProvider — round-trip, missing files, nested writes.

#include "Core/IO/IFile.h"
#include "Core/IO/Virtual/Providers/LooseFileProvider.h"

#include <doctest/doctest.h>

#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

using namespace Hylux;

namespace
{

std::filesystem::path MakeTempDir()
{
    const auto base = std::filesystem::temp_directory_path();
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    auto dir = base / ("hylux-vfs-test-" + std::to_string(stamp));
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

void RemoveAllSilent(const std::filesystem::path& p)
{
    std::error_code ec;
    std::filesystem::remove_all(p, ec);
}

void WriteAll(IFile& f, std::string_view bytes)
{
    f.Write(bytes.data(), bytes.size());
}

std::string ReadAll(IFile& f)
{
    std::string out;
    std::array<char, 256> buf{};
    while (true)
    {
        const auto n = f.Read(buf.data(), buf.size());
        if (n == 0) break;
        out.append(buf.data(), static_cast<std::size_t>(n));
    }
    return out;
}

} // namespace

TEST_CASE("LooseFileProvider reads back a file written through the physical layer")
{
    const auto tempDir = MakeTempDir();

    {
        std::ofstream out(tempDir / "hello.txt", std::ios::binary);
        out << "hello vfs";
    }

    LooseFileProvider provider(tempDir);
    CHECK(provider.SupportsWrite());
    CHECK(provider.Exists("hello.txt"));
    CHECK_FALSE(provider.Exists("missing.txt"));

    auto file = provider.Open("hello.txt", FileOpenMode::Read);
    REQUIRE(file != nullptr);
    CHECK(ReadAll(*file) == "hello vfs");

    CHECK(provider.Open("missing.txt", FileOpenMode::Read) == nullptr);

    RemoveAllSilent(tempDir);
}

TEST_CASE("LooseFileProvider auto-creates parent directories on Write")
{
    const auto tempDir = MakeTempDir();
    LooseFileProvider provider(tempDir);

    auto writer = provider.Open("nested/dir/new.txt", FileOpenMode::Write);
    REQUIRE(writer != nullptr);
    WriteAll(*writer, "wrote-through-vfs");
    writer.reset();

    CHECK(provider.Exists("nested/dir/new.txt"));

    auto reader = provider.Open("nested/dir/new.txt", FileOpenMode::Read);
    REQUIRE(reader != nullptr);
    CHECK(ReadAll(*reader) == "wrote-through-vfs");

    RemoveAllSilent(tempDir);
}

TEST_CASE("LooseFileProvider Stat reports size and file kind")
{
    const auto tempDir = MakeTempDir();
    {
        std::ofstream out(tempDir / "x.bin", std::ios::binary);
        out << "1234567890";
    }
    LooseFileProvider provider(tempDir);

    const auto stat = provider.Stat("x.bin");
    CHECK(stat.exists);
    CHECK(stat.isRegularFile);
    CHECK_FALSE(stat.isDirectory);
    CHECK(stat.size == 10);

    RemoveAllSilent(tempDir);
}
