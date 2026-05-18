/// @file
/// @brief ThreadAffinity backing storage. Two thread_local variables; that's the entire
///        implementation. Cost on the fast path is one TLS load.

#include "Core/Concurrency/ThreadAffinity.h"

namespace Hylux::Concurrency
{

namespace
{
thread_local ThreadRole tlsRole_       = ThreadRole::Unknown;
thread_local int        tlsWorkerIdx_  = -1;
} // namespace

void RegisterCurrentThreadRole(ThreadRole role) noexcept
{
    tlsRole_ = role;
}

ThreadRole GetCurrentThreadRole() noexcept
{
    return tlsRole_;
}

int GetCurrentWorkerIndex() noexcept
{
    return tlsWorkerIdx_;
}

void SetCurrentWorkerIndex(int index) noexcept
{
    tlsWorkerIdx_ = index;
}

} // namespace Hylux::Concurrency
