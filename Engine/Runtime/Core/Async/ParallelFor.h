/// @file
/// @brief Parallel loop helpers built on TaskGraph. ParallelFor is the convenience entry
///        point for fire-and-await CPU fan-out; TaskGraph::AddParallelFor lets you embed a
///        parallel loop as one logical node inside a larger DAG.

#pragma once

#include "Core/Async/IExecutor.h"
#include "Core/Async/TaskGraph.h"
#include "Core/Async/Unit.h"

#include <cstddef>
#include <functional>
#include <thread>
#include <utility>

namespace Hylux
{

namespace Detail
{
inline std::size_t ResolveChunkSize(std::size_t total, std::size_t chunkSize) noexcept
{
    if (total == 0)
    {
        return 0;
    }
    if (chunkSize > 0)
    {
        return chunkSize;
    }
    const unsigned    hw     = std::thread::hardware_concurrency();
    const std::size_t target = hw > 0 ? static_cast<std::size_t>(hw) * 2u : 8u;
    const std::size_t derived = (total + target - 1) / target;
    return derived > 0 ? derived : 1;
}
} // namespace Detail

/// @brief Splits [begin, end) into chunks and runs @p body(i) for each i across @p exec
///        workers. Returns a Future<Unit> that resolves once every iteration has completed.
///        chunkSize == 0 picks a default proportional to hardware_concurrency.
[[nodiscard]] inline Future<Unit> ParallelFor(IExecutor& exec, std::size_t begin, std::size_t end,
                                              std::function<void(std::size_t)> body,
                                              std::size_t chunkSize = 0)
{
    if (begin >= end)
    {
        return Future<Unit>::MakeReady(Unit{});
    }
    const std::size_t total  = end - begin;
    const std::size_t chunk  = Detail::ResolveChunkSize(total, chunkSize);
    TaskGraph         g;
    for (std::size_t start = begin; start < end; start += chunk)
    {
        const std::size_t stop = start + chunk < end ? start + chunk : end;
        g.AddNode("parallel-for-chunk",
                  [body, start, stop](TaskContext& ctx) {
                      for (std::size_t i = start; i < stop; ++i)
                      {
                          if (ctx.Cancellation().IsCanceled())
                          {
                              return;
                          }
                          body(i);
                      }
                  });
    }
    return g.Submit(exec);
}

} // namespace Hylux
