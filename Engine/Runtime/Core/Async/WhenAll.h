/// @file
/// @brief WhenAll — combinator that returns a single Future fulfilled once every input
///        Future has resolved. The result vector preserves input order. Empty input is
///        handled as an already-ready future with an empty result.

#pragma once

#include "Core/Async/Future.h"
#include "Core/Memory/MakeRef.h"
#include "Core/Memory/Ref.h"
#include "Core/Memory/RefCounted.h"

#include <atomic>
#include <cstddef>
#include <utility>
#include <vector>

namespace Hylux
{

namespace Detail
{

template<typename T>
class WhenAllState final : public RefCounted
{
public:
    WhenAllState(std::size_t count) : results_(count), remaining_(count) {}

    void Write(std::size_t index, const T& value)
    {
        std::lock_guard lock(mutex_);
        results_[index] = value;
    }

    void CompleteOne()
    {
        if (remaining_.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            std::vector<T> snapshot;
            {
                std::lock_guard lock(mutex_);
                snapshot.swap(results_);
            }
            promise_.Set(std::move(snapshot));
        }
    }

    [[nodiscard]] Promise<std::vector<T>>& OutPromise() noexcept { return promise_; }

private:
    std::mutex               mutex_;
    std::vector<T>           results_;
    std::atomic<std::size_t> remaining_;
    Promise<std::vector<T>>  promise_;
};

} // namespace Detail

/// @brief Returns a Future that resolves when every input future has resolved. The
///        resolved vector preserves input order. T must be copyable (each input is
///        copied into the result slot inside the input's continuation). The internal
///        write path takes a mutex so std::vector<bool>'s bitfield aliasing does not
///        race when multiple inputs complete concurrently — contention is bounded by
///        the input count of a single WhenAll, so this is fine in practice.
template<typename T>
[[nodiscard]] Future<std::vector<T>> WhenAll(std::vector<Future<T>> futures)
{
    if (futures.empty())
    {
        return Future<std::vector<T>>::MakeReady({});
    }

    auto state = MakeRef<Detail::WhenAllState<T>>(futures.size());
    for (std::size_t i = 0; i < futures.size(); ++i)
    {
        futures[i].Then([state, i](T& value) {
            state->Write(i, value);
            state->CompleteOne();
        });
    }
    return state->OutPromise().GetFuture();
}

} // namespace Hylux
