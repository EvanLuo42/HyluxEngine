/// @file
/// @brief FileSink implementation.

#include "Core/Logging/Sinks/FileSink.h"

#include <chrono>
#include <format>
#include <ios>
#include <iterator>
#include <string>
#include <system_error>

namespace Hylux
{
namespace
{

std::string MakeFilename(std::chrono::system_clock::time_point now)
{
    using namespace std::chrono;
    const auto local = current_zone()->to_local(now);
    const auto day   = floor<days>(local);
    const year_month_day ymd{day};
    const auto tod   = hh_mm_ss{floor<seconds>(local - day)};
    return std::format("Hylux-{:04}{:02}{:02}-{:02}{:02}{:02}.log",
        static_cast<int>(ymd.year()),
        static_cast<unsigned>(ymd.month()),
        static_cast<unsigned>(ymd.day()),
        tod.hours().count(), tod.minutes().count(), tod.seconds().count());
}

void FormatLine(std::string& out, const LogRecord& record)
{
    using namespace std::chrono;
    const auto local = current_zone()->to_local(record.timestamp);
    const auto day   = floor<days>(local);
    const year_month_day ymd{day};
    const auto tod   = hh_mm_ss{floor<milliseconds>(local - day)};
    std::format_to(std::back_inserter(out),
        "{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03} [{}] [{}] {}:{} {}\n",
        static_cast<int>(ymd.year()),
        static_cast<unsigned>(ymd.month()),
        static_cast<unsigned>(ymd.day()),
        tod.hours().count(), tod.minutes().count(), tod.seconds().count(),
        tod.subseconds().count(),
        LevelToString(record.level),
        record.categoryName,
        record.site.file, record.site.line,
        record.message);
}

} // namespace

FileSink::FileSink(const std::filesystem::path& directory)
{
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    filePath_ = directory / MakeFilename(std::chrono::system_clock::now());
    stream_.open(filePath_, std::ios::out | std::ios::trunc);
    if (!stream_.is_open())
    {
        filePath_.clear();
    }
}

FileSink::~FileSink()
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (stream_.is_open())
    {
        stream_.flush();
        stream_.close();
    }
}

void FileSink::Submit(const LogRecord& record)
{
    std::string line;
    line.reserve(160 + record.message.size());
    FormatLine(line, record);

    std::lock_guard<std::mutex> guard(mutex_);
    if (stream_.is_open())
    {
        stream_.write(line.data(), static_cast<std::streamsize>(line.size()));
    }
}

void FileSink::Flush()
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (stream_.is_open())
    {
        stream_.flush();
    }
}

} // namespace Hylux
