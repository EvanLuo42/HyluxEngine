/// @file
/// @brief TaskGraph implementation. Submit materializes the Execution object, transfers all
///        node closures into it, then starts every root node. Each running node decrements
///        its successors' remaining-dep counters and dispatches them when they hit zero.

#include "Core/Async/TaskGraph.h"

#include "Core/Async/ParallelFor.h"
#include "Core/Concurrency/ThreadAffinity.h"
#include "Core/Utils/Assert.h"

#include <utility>
#include <vector>

namespace Hylux
{

namespace Detail
{

void TaskGraphExecution::Dispatch(Ref<TaskGraphExecution> self, std::uint32_t nodeIdx)
{
    Node& node = self->nodes_[nodeIdx];
    IExecutor* exec = node.executor != nullptr ? node.executor : self->defaultExecutor_;
    HYLUX_ASSERT_MSG(exec != nullptr, "TaskGraphExecution::Dispatch: no executor available for node");
    exec->Post([self, nodeIdx]() mutable { self->Run(nodeIdx); });
}

void TaskGraphExecution::Run(std::uint32_t nodeIdx)
{
    Node& node = nodes_[nodeIdx];
    const bool canceled = token_.IsCanceled();
    if (!canceled && node.body)
    {
        TaskContext ctx(token_, Concurrency::GetCurrentWorkerIndex());
        try
        {
            node.body(ctx);
        }
        catch (...)
        {
        }
    }
    OnNodeComplete(nodeIdx);
}

void TaskGraphExecution::OnNodeComplete(std::uint32_t nodeIdx)
{
    Ref<TaskGraphExecution> self(this);
    Node&                   node = nodes_[nodeIdx];
    for (std::uint32_t succ : node.successors)
    {
        const std::uint32_t prev =
            nodes_[succ].remainingDeps.fetch_sub(1, std::memory_order_acq_rel);
        if (prev == 1)
        {
            Dispatch(self, succ);
        }
    }
    const std::uint32_t remaining = remainingNodes_.fetch_sub(1, std::memory_order_acq_rel);
    if (remaining == 1)
    {
        promise_.Set(Unit{});
    }
}

} // namespace Detail

TaskNodeId TaskGraph::AddParallelFor(std::string name, std::span<const TaskNodeId> deps,
                                     std::size_t begin, std::size_t end,
                                     std::function<void(std::size_t)> body, std::size_t chunkSize)
{
    if (begin >= end)
    {
        return AddNode(std::move(name), deps, [](TaskContext&) {});
    }
    const std::size_t total = end - begin;
    const std::size_t chunk = Detail::ResolveChunkSize(total, chunkSize);

    std::vector<TaskNodeId> chunkIds;
    chunkIds.reserve((total + chunk - 1) / chunk);
    for (std::size_t start = begin; start < end; start += chunk)
    {
        const std::size_t stop = start + chunk < end ? start + chunk : end;
        TaskNodeId        id   = AddNode(name + "-chunk", deps,
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
        chunkIds.push_back(id);
    }
    return AddNode(std::move(name), chunkIds, [](TaskContext&) {});
}

Future<Unit> TaskGraph::Submit(IExecutor& defaultExec, CancellationToken token)
{
    HYLUX_ASSERT_MSG(!submitted_, "TaskGraph::Submit called more than once");
    submitted_ = true;

    auto execution = MakeRef<Detail::TaskGraphExecution>(static_cast<std::uint32_t>(builders_.size()));
    execution->SetDefaultExecutor(defaultExec);
    execution->SetCancellationToken(std::move(token));

    const std::uint32_t n = static_cast<std::uint32_t>(builders_.size());
    for (std::uint32_t i = 0; i < n; ++i)
    {
        auto&                            node = execution->GetNode(i);
        Builder&                         src  = builders_[i];
        node.name     = std::move(src.name);
        node.body     = std::move(src.body);
        node.executor = src.executor;
        node.remainingDeps.store(static_cast<std::uint32_t>(src.dependsOn.size()),
                                  std::memory_order_relaxed);
    }
    for (std::uint32_t i = 0; i < n; ++i)
    {
        for (TaskNodeId dep : builders_[i].dependsOn)
        {
            HYLUX_ASSERT_MSG(dep.IsValid() && dep.value < n, "TaskGraph dependency out of range");
            execution->GetNode(dep.value).successors.push_back(i);
        }
    }

    Future<Unit> fut = execution->GetFuture();
    execution->Start();
    builders_.clear();
    return fut;
}

} // namespace Hylux
