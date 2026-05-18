/// @file
/// @brief CancellationSource / CancellationToken — a cooperative cancellation primitive.
///        The owner of a unit of work holds a CancellationSource and can flip it once;
///        every worker passes a CancellationToken that periodically polls IsCanceled at
///        safe checkpoints (before allocating staging, before submitting a queue, between
///        loop iterations, …) and bails out early when the flip is observed. Cancellation
///        is advisory — the runtime does not interrupt in-flight system calls; it only
///        guarantees that a Cancel() call eventually becomes visible to subsequent token
///        polls.

#pragma once

#include "Core/Memory/MakeRef.h"
#include "Core/Memory/Ref.h"
#include "Core/Memory/RefCounted.h"

#include <atomic>

namespace Hylux
{

namespace Detail
{

class CancellationState final : public RefCounted
{
public:
    void Cancel() noexcept { canceled_.store(true, std::memory_order_release); }

    [[nodiscard]] bool IsCanceled() const noexcept
    {
        return canceled_.load(std::memory_order_acquire);
    }

private:
    std::atomic<bool> canceled_{false};
};

} // namespace Detail

/// @brief Read side of the cancellation primitive. Default-constructed tokens reference
///        no state and never report canceled — pass one of these as the default argument
///        on async APIs that don't yet need a real source.
class CancellationToken
{
public:
    CancellationToken() noexcept = default;

    [[nodiscard]] bool IsCanceled() const noexcept
    {
        return state_ && state_->IsCanceled();
    }

    [[nodiscard]] bool IsValid() const noexcept { return static_cast<bool>(state_); }

private:
    friend class CancellationSource;

    explicit CancellationToken(Ref<Detail::CancellationState> state) noexcept
        : state_(std::move(state))
    {}

    Ref<Detail::CancellationState> state_;
};

/// @brief Write side. Construct one, hand its token to workers, call Cancel() at most
///        once when you no longer want the work to complete. Cancellation is sticky —
///        once flipped, every subsequent token poll observes canceled.
class CancellationSource
{
public:
    CancellationSource() : state_(MakeRef<Detail::CancellationState>()) {}

    void Cancel() noexcept { state_->Cancel(); }

    [[nodiscard]] bool              IsCanceled() const noexcept { return state_->IsCanceled(); }
    [[nodiscard]] CancellationToken GetToken() const { return CancellationToken{state_}; }

private:
    Ref<Detail::CancellationState> state_;
};

} // namespace Hylux
