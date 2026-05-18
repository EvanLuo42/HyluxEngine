/// @file
/// @brief Copy-queue RenderGraph pass. Records CopyBuffer / CopyTexture / CopyBufferToTexture
///        in Execute and asks the scheduler to land the work on the dedicated copy queue when
///        the device exposes one (transfers run in parallel with graphics / compute on PCIe
///        and console DMA engines).

#pragma once

#include "Core/Reflection/ReflectionMacros.h"
#include "RenderGraph/RGPass.h"

namespace Hylux::RG
{

/// @brief Specialization of RGPass for transfer work. The base Setup(RGBuilder&) is the right
///        surface — copy passes only ever declare reads (TransferRead, TransferSrc layout) and
///        writes (TransferWrite, TransferDst layout). Stronger validation belongs in a future
///        RGCopyBuilder; the base builder is intentionally permissive for now so streaming /
///        upload code can be staged here without waiting on the queue scheduler.
class RGCopyPass : public RGPass
{
    HYLUX_REFLECT_BODY(RGCopyPass)
public:
    [[nodiscard]] QueueAffinity GetQueueAffinity() const noexcept override
    {
        return QueueAffinity::Copy;
    }
};

} // namespace Hylux::RG
