/// @file
/// @brief Named-node DAG built on top of Future / IExecutor / CancellationToken. Build a
///        graph by calling AddNode (optionally with dependencies and an executor pin), then
///        Submit to schedule it. Submit returns a Future<Unit> that resolves once every node
///        has finished — whether by running its body, by being skipped due to cancellation,
///        or by being a roots-only no-op graph.

#pragma once

#include "Core/Async/CancellationToken.h"
#include "Core/Async/Future.h"
#include "Core/Async/IExecutor.h"
#include "Core/Async/Unit.h"
#include "Core/Memory/MakeRef.h"
#include "Core/Memory/Ref.h"
#include "Core/Memory/RefCounted.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace Hylux
{

/// @brief Strongly-typed identifier for a node added to a TaskGraph. Treat as opaque.
struct TaskNodeId
{
    std::uint32_t value{static_cast<std::uint32_t>(-1)};

    [[nodiscard]] bool IsValid() const noexcept { return value != static_cast<std::uint32_t>(-1); }
};

/// @brief Per-invocation context passed to a node body. Carries the graph's cancellation
///        token (so the body can poll at safe points) and the worker index of the executing
///        thread (or -1 if the body runs on a non-worker thread).
class TaskContext
{
public:
    TaskContext(CancellationToken cancellation, int workerIndex) noexcept
        : cancellation_(std::move(cancellation)), workerIndex_(workerIndex)
    {
    }

    [[nodiscard]] const CancellationToken& Cancellation() const noexcept { return cancellation_; }
    [[nodiscard]] int                      WorkerIndex() const noexcept { return workerIndex_; }

private:
    CancellationToken cancellation_;
    int               workerIndex_;
};

namespace Detail
{

class TaskGraphExecution : public RefCounted
{
public:
    struct Node
    {
        std::string                              name;
        std::function<void(TaskContext&)>        body;
        IExecutor*                               executor{nullptr};
        std::vector<std::uint32_t>               successors;
        std::atomic<std::uint32_t>               remainingDeps{0};
    };

    explicit TaskGraphExecution(std::uint32_t nodeCount)
        : nodes_(nodeCount), remainingNodes_(nodeCount), promise_()
    {
    }

    [[nodiscard]] Node&         GetNode(std::uint32_t index) { return nodes_[index]; }
    [[nodiscard]] Future<Unit>  GetFuture() const { return promise_.GetFuture(); }

    void SetDefaultExecutor(IExecutor& exec) noexcept { defaultExecutor_ = &exec; }
    void SetCancellationToken(CancellationToken token) noexcept { token_ = std::move(token); }

    void Start()
    {
        if (nodes_.empty())
        {
            promise_.Set(Unit{});
            return;
        }
        Ref<TaskGraphExecution> self(this);
        for (std::uint32_t i = 0; i < nodes_.size(); ++i)
        {
            if (nodes_[i].remainingDeps.load(std::memory_order_relaxed) == 0)
            {
                Dispatch(self, i);
            }
        }
    }

private:
    static void Dispatch(Ref<TaskGraphExecution> self, std::uint32_t nodeIdx);
    void        Run(std::uint32_t nodeIdx);
    void        OnNodeComplete(std::uint32_t nodeIdx);

    std::vector<Node>          nodes_;
    std::atomic<std::uint32_t> remainingNodes_;
    Promise<Unit>              promise_;
    IExecutor*                 defaultExecutor_{nullptr};
    CancellationToken          token_;
};

} // namespace Detail

/// @brief Builder for a named-node DAG. Build with AddNode, then call Submit exactly once.
class TaskGraph
{
public:
    TaskGraph()  = default;
    ~TaskGraph() = default;

    TaskGraph(const TaskGraph&)            = delete;
    TaskGraph& operator=(const TaskGraph&) = delete;

    TaskGraph(TaskGraph&&) noexcept            = default;
    TaskGraph& operator=(TaskGraph&&) noexcept = default;

    /// @brief Adds a node with no dependencies, dispatched to the default executor.
    TaskNodeId AddNode(std::string name, std::function<void(TaskContext&)> body)
    {
        return AddNodeOn(nullptr, std::move(name), {}, std::move(body));
    }

    /// @brief Adds a node with dependencies, dispatched to the default executor.
    TaskNodeId AddNode(std::string name, std::span<const TaskNodeId> deps,
                       std::function<void(TaskContext&)> body)
    {
        return AddNodeOn(nullptr, std::move(name), deps, std::move(body));
    }

    /// @brief Adds a node pinned to @p exec. Pass nullptr to fall back to Submit's default
    ///        executor.
    TaskNodeId AddNodeOn(IExecutor* exec, std::string name, std::span<const TaskNodeId> deps,
                         std::function<void(TaskContext&)> body)
    {
        Builder b;
        b.name       = std::move(name);
        b.body       = std::move(body);
        b.executor   = exec;
        b.dependsOn.assign(deps.begin(), deps.end());
        const std::uint32_t idx = static_cast<std::uint32_t>(builders_.size());
        builders_.push_back(std::move(b));
        return TaskNodeId{idx};
    }

    /// @brief Adds a parallel-for region as a single logical node. Internally creates one
    ///        chunk node per [start, stop) slice (all dispatched to the default executor) and
    ///        a join node that depends on every chunk. The returned id is the join node;
    ///        downstream nodes can depend on it to sequence after the loop. chunkSize == 0
    ///        picks a default proportional to hardware_concurrency.
    TaskNodeId AddParallelFor(std::string name, std::span<const TaskNodeId> deps, std::size_t begin,
                              std::size_t end, std::function<void(std::size_t)> body,
                              std::size_t chunkSize = 0);

    /// @brief Schedules every node onto its target executor (resolved against @p defaultExec),
    ///        returns a future that resolves once the whole graph has drained.
    ///        Calling Submit more than once is a programmer error.
    [[nodiscard]] Future<Unit> Submit(IExecutor& defaultExec, CancellationToken token = {});

    [[nodiscard]] std::size_t NodeCount() const noexcept { return builders_.size(); }

private:
    struct Builder
    {
        std::string                              name;
        std::function<void(TaskContext&)>        body;
        IExecutor*                               executor{nullptr};
        std::vector<TaskNodeId>                  dependsOn;
    };

    std::vector<Builder> builders_;
    bool                 submitted_{false};
};

} // namespace Hylux
