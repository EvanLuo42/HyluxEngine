/// @file
/// @brief PhysicalFileSystem::Create dispatch — picks the platform's IFileSystem implementation.

#include "Core/IO/PhysicalFileSystem.h"

#include "Core/IO/IFileSystem.h"

#if defined(HYLUX_DESKTOP)
    #include "Core/IO/Platform/Desktop/StdFileSystem.h"
#endif

namespace Hylux
{

std::unique_ptr<IFileSystem> PhysicalFileSystem::Create()
{
#if defined(HYLUX_DESKTOP)
    return std::make_unique<StdFileSystem>();
#else
    return nullptr;
#endif
}

} // namespace Hylux
