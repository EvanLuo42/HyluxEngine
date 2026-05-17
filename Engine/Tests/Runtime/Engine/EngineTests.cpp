/// @file
/// @brief Tests for Engine subsystem lifecycle + tickable scheduling.

#include "Engine/Engine.h"
#include "Engine/ISubsystem.h"
#include "Engine/ITickable.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Engine");

#include <string>
#include <vector>

using namespace Hylux;

namespace
{

class TraceSubsystem : public ISubsystem
{
public:
    explicit TraceSubsystem(std::string name, std::vector<std::string>* trace,
                            std::vector<TypeId> deps = {})
        : name_(std::move(name)), trace_(trace), deps_(std::move(deps))
    {}

    void Initialize(Engine&) override
    {
        if (trace_) trace_->push_back("init:" + name_);
        initialized_ = true;
    }
    void Shutdown() override
    {
        if (trace_) trace_->push_back("shut:" + name_);
        initialized_ = false;
    }
    [[nodiscard]] std::vector<TypeId> GetDependencies() const override { return deps_; }
    [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }

private:
    std::string                name_;
    std::vector<std::string>*  trace_;
    std::vector<TypeId>        deps_;
    bool                       initialized_{false};
};

class AlphaSubsystem : public TraceSubsystem
{
public:
    explicit AlphaSubsystem(std::vector<std::string>* trace)
        : TraceSubsystem("alpha", trace) {}
};

class BetaSubsystem : public TraceSubsystem
{
public:
    explicit BetaSubsystem(std::vector<std::string>* trace)
        : TraceSubsystem("beta", trace, {TypeIdOf<AlphaSubsystem>()}) {}
};

class GammaSubsystem : public TraceSubsystem
{
public:
    explicit GammaSubsystem(std::vector<std::string>* trace)
        : TraceSubsystem("gamma", trace, {TypeIdOf<BetaSubsystem>()}) {}
};

class CountingTickable : public ITickable
{
public:
    explicit CountingTickable(int order = 0) : order_(order) {}
    void Tick(float dt) override { ++count_; lastDt_ = dt; }
    [[nodiscard]] int TickOrder() const override { return order_; }

    int   count_{0};
    float lastDt_{0.0f};

private:
    int order_;
};

} // namespace

// ---- Subsystem registration / dependency ordering ------------------------

TEST_CASE("Engine: not initialized after construction; GetSubsystem returns nullptr for missing")
{
    Engine engine;
    CHECK_FALSE(engine.IsInitialized());
    CHECK(engine.GetSubsystem<AlphaSubsystem>() == nullptr);
}

TEST_CASE("Engine::RegisterSubsystem + GetSubsystem returns the registered instance")
{
    Engine engine;
    auto* alpha = engine.RegisterSubsystem<AlphaSubsystem>(nullptr);
    REQUIRE(alpha != nullptr);
    CHECK(engine.GetSubsystem<AlphaSubsystem>() == alpha);
}

TEST_CASE("Engine::Initialize: respects topological order from GetDependencies; Shutdown reverses it")
{
    std::vector<std::string> trace;
    Engine engine;
    engine.RegisterSubsystem<GammaSubsystem>(&trace);
    engine.RegisterSubsystem<AlphaSubsystem>(&trace);
    engine.RegisterSubsystem<BetaSubsystem>(&trace);

    engine.Initialize();
    CHECK(engine.IsInitialized());
    REQUIRE(trace.size() == 3u);
    CHECK(trace[0] == "init:alpha");
    CHECK(trace[1] == "init:beta");
    CHECK(trace[2] == "init:gamma");

    engine.Shutdown();
    CHECK_FALSE(engine.IsInitialized());
    REQUIRE(trace.size() == 6u);
    CHECK(trace[3] == "shut:gamma");
    CHECK(trace[4] == "shut:beta");
    CHECK(trace[5] == "shut:alpha");
}

TEST_CASE("Engine::Shutdown is a no-op when not initialized")
{
    Engine engine;
    engine.Shutdown();
    engine.Shutdown();  // double shutdown is also safe
    CHECK_FALSE(engine.IsInitialized());
}

// ---- Tickables ----------------------------------------------------------

TEST_CASE("Engine::Tick: registered tickables run; nullptr register/unregister are no-ops")
{
    Engine engine;
    engine.Initialize();
    CountingTickable t;
    engine.RegisterTickable(&t);
    engine.RegisterTickable(nullptr);
    engine.Tick(0.5f);
    CHECK(t.count_ == 1);
    CHECK(t.lastDt_ == 0.5f);

    engine.UnregisterTickable(&t);
    engine.UnregisterTickable(nullptr);
    engine.Tick(0.25f);
    CHECK(t.count_ == 1);
    engine.Shutdown();
}

TEST_CASE("Engine::Tick: stable sort by TickOrder preserves registration order on ties")
{
    Engine engine;
    engine.Initialize();
    CountingTickable a{10}, b{5}, c{10};
    engine.RegisterTickable(&a);
    engine.RegisterTickable(&b);
    engine.RegisterTickable(&c);
    engine.Tick(0.1f);
    CHECK(a.count_ == 1);
    CHECK(b.count_ == 1);
    CHECK(c.count_ == 1);
    engine.Shutdown();
}

TEST_CASE("Engine: register + unregister in the same frame cancels the registration")
{
    Engine engine;
    engine.Initialize();
    CountingTickable t;
    engine.RegisterTickable(&t);
    engine.UnregisterTickable(&t);
    engine.Tick(0.1f);
    CHECK(t.count_ == 0);
    engine.Shutdown();
}

TEST_SUITE_END();
