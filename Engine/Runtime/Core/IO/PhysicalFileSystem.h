/// @file
/// @brief Factory that produces the platform's physical (disk-backed) IFileSystem implementation.

#pragma once

#include <memory>

namespace Hylux
{

class IFileSystem;

/// @brief Factory for the platform-native physical filesystem. Picks the concrete implementation
///        at compile time based on the active platform (HYLUX_DESKTOP / HYLUX_PLATFORM_* macros).
///        Returns nullptr on platforms that do not yet have an implementation.
class PhysicalFileSystem
{
public:
    PhysicalFileSystem()                                     = delete;
    PhysicalFileSystem(const PhysicalFileSystem&)            = delete;
    PhysicalFileSystem& operator=(const PhysicalFileSystem&) = delete;

    /// @brief Creates a new physical filesystem instance for the current platform.
    static std::unique_ptr<IFileSystem> Create();
};

} // namespace Hylux
