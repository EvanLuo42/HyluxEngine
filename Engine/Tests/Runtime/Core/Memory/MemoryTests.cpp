/// @file
/// @brief Tests for the intrusive refcounting facilities: Ref / WeakRef / RefCounted /
///        MakeRef / EnableRefFromThis.

#include "Core/Memory/Memory.h"

#include <doctest/doctest.h>

#include <utility>

using namespace Hylux;

namespace
{

/// @brief Trivial RefCounted subclass that pings a counter on destruction so tests can
///        observe object lifetime independently of strong/weak counts.
class Probe : public RefCounted
{
public:
    explicit Probe(int* destructions) noexcept : destructions_(destructions) {}
    ~Probe() override
    {
        if (destructions_ != nullptr)
        {
            ++(*destructions_);
        }
    }

    int payload_{42};

private:
    int* destructions_;
};

class DerivedProbe : public Probe
{
public:
    explicit DerivedProbe(int* destructions) noexcept : Probe(destructions) {}
};

class SharedProbe : public RefCounted, public EnableRefFromThis<SharedProbe>
{
public:
    int value_{7};
};

} // namespace

TEST_CASE("MakeRef yields the sole strong reference")
{
    int destructions = 0;
    {
        Ref<Probe> r = MakeRef<Probe>(&destructions);
        REQUIRE(r);
        CHECK(r->GetStrongCount() == 1u);
        CHECK(r->GetWeakCount() == 1u);
        CHECK(r->payload_ == 42);
        CHECK(destructions == 0);
    }
    CHECK(destructions == 1);
}

TEST_CASE("Copying a Ref bumps the strong count, destroying both releases the object")
{
    int destructions = 0;
    {
        Ref<Probe> a = MakeRef<Probe>(&destructions);
        {
            Ref<Probe> b = a;
            CHECK(a.Get() == b.Get());
            CHECK(a->GetStrongCount() == 2u);
        }
        CHECK(a->GetStrongCount() == 1u);
        CHECK(destructions == 0);
    }
    CHECK(destructions == 1);
}

TEST_CASE("Move construction transfers ownership without bumping the strong count")
{
    int destructions = 0;
    Ref<Probe> source = MakeRef<Probe>(&destructions);
    Probe* raw = source.Get();
    REQUIRE(raw->GetStrongCount() == 1u);

    Ref<Probe> moved = std::move(source);
    CHECK(source.Get() == nullptr);
    CHECK(moved.Get() == raw);
    CHECK(raw->GetStrongCount() == 1u);
}

TEST_CASE("Reset releases the held object and clears the pointer")
{
    int destructions = 0;
    Ref<Probe> r = MakeRef<Probe>(&destructions);
    r.Reset();
    CHECK(r.Get() == nullptr);
    CHECK(destructions == 1);
}

TEST_CASE("Detach yields a +1 raw pointer that Attach can re-adopt")
{
    int destructions = 0;
    Ref<Probe> r = MakeRef<Probe>(&destructions);
    Probe* raw = r.Detach();
    REQUIRE(raw != nullptr);
    CHECK(r.Get() == nullptr);
    CHECK(raw->GetStrongCount() == 1u);

    Ref<Probe> adopted;
    adopted.Attach(raw);
    CHECK(adopted.Get() == raw);
    CHECK(adopted->GetStrongCount() == 1u);
    CHECK(destructions == 0);
}

TEST_CASE("Comparison operators compare the underlying pointer")
{
    int d = 0;
    Ref<Probe> a = MakeRef<Probe>(&d);
    Ref<Probe> b = a;
    Ref<Probe> c = MakeRef<Probe>(&d);
    CHECK(a == b);
    CHECK(a != c);
    CHECK(Ref<Probe>{} == nullptr);
    CHECK(nullptr == Ref<Probe>{});
    CHECK(a != nullptr);
}

TEST_CASE("Ref converts from a derived Ref to a base Ref")
{
    int destructions = 0;
    Ref<DerivedProbe> derived = MakeRef<DerivedProbe>(&destructions);
    Ref<Probe> base = derived;
    CHECK(base.Get() == derived.Get());
    CHECK(derived->GetStrongCount() == 2u);
}

TEST_CASE("WeakRef does not extend object lifetime but keeps the control block addressable")
{
    int destructions = 0;
    WeakRef<Probe> weak;
    {
        Ref<Probe> strong = MakeRef<Probe>(&destructions);
        weak = strong;
        CHECK_FALSE(weak.Expired());
        CHECK(strong->GetWeakCount() == 2u);

        Ref<Probe> locked = weak.Lock();
        CHECK(locked.Get() == strong.Get());
        CHECK(strong->GetStrongCount() == 2u);
    }
    CHECK(destructions == 1);
    CHECK(weak.Expired());
    CHECK(weak.Lock().Get() == nullptr);
}

TEST_CASE("WeakRef::Lock from a never-assigned weak returns an empty Ref")
{
    WeakRef<Probe> weak;
    CHECK(weak.Expired());
    CHECK(weak.Lock().Get() == nullptr);
}

TEST_CASE("EnableRefFromThis upgrades this to a Ref while the object is alive")
{
    Ref<SharedProbe> owner = MakeRef<SharedProbe>();
    Ref<SharedProbe> fromThis = owner->RefFromThis();
    REQUIRE(fromThis);
    CHECK(fromThis.Get() == owner.Get());
    CHECK(owner->GetStrongCount() == 2u);
}
