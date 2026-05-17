/// @file
/// @brief Tests for the intrusive refcounting facilities: Ref / WeakRef / RefCounted /
///        MakeRef / EnableRefFromThis.

#include "Core/Memory/Memory.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Core::Memory");

#include <utility>

using namespace Hylux;

namespace
{

class Probe : public RefCounted
{
public:
    explicit Probe(int* destructions = nullptr) noexcept : destructions_(destructions) {}
    ~Probe() override
    {
        if (destructions_ != nullptr) ++(*destructions_);
    }
    int payload_{42};

private:
    int* destructions_;
};

class DerivedProbe : public Probe
{
public:
    explicit DerivedProbe(int* destructions = nullptr) noexcept : Probe(destructions) {}
};

class SharedProbe : public RefCounted, public EnableRefFromThis<SharedProbe>
{
public:
    int value_{7};
};

} // namespace

// ---- RefCounted ------------------------------------------------------------

TEST_CASE("RefCounted: initial state (strong=0, weak=1) and counter accessors")
{
    auto* raw = new Probe;
    CHECK(raw->GetStrongCount() == 0u);
    CHECK(raw->GetWeakCount() == 1u);
    raw->AddRef();
    CHECK(raw->GetStrongCount() == 1u);
    raw->Release();  // drops to 0, runs dtor, releases phantom weak, frees storage
}

TEST_CASE("RefCounted: AddWeakRef / ReleaseWeakRef and storage-free path")
{
    int destroyed = 0;
    auto* raw = new Probe(&destroyed);
    raw->AddRef();
    raw->AddWeakRef();  // weak=2
    raw->Release();     // strong->0 runs dtor; weak->1 (phantom released)
    CHECK(destroyed == 1);
    // raw is still addressable because we hold one explicit weak ref.
    CHECK(raw->GetWeakCount() == 1u);
    raw->ReleaseWeakRef();  // weak->0 frees storage; raw now dangling (do not dereference)
}

TEST_CASE("RefCounted::TryAddRefFromWeak: succeeds while live, fails after strong=0")
{
    auto* raw = new Probe;
    raw->AddRef();
    raw->AddWeakRef();
    CHECK(raw->TryAddRefFromWeak());
    CHECK(raw->GetStrongCount() == 2u);
    raw->Release();
    raw->Release();  // strong -> 0; dtor runs; phantom released; weak = 1 (our explicit one)
    CHECK_FALSE(raw->TryAddRefFromWeak());
    raw->ReleaseWeakRef();
}

// ---- Ref<T> ----------------------------------------------------------------

TEST_CASE("Ref: default and nullptr ctors are empty")
{
    Ref<Probe> a;
    Ref<Probe> b{nullptr};
    CHECK(a.Get() == nullptr);
    CHECK(b.Get() == nullptr);
    CHECK_FALSE(a);
    CHECK(a == nullptr);
    CHECK(nullptr == a);
}

TEST_CASE("Ref(raw): AddRefs on construction; dtor Releases")
{
    int destroyed = 0;
    {
        auto* raw = new Probe(&destroyed);
        raw->AddRef();
        Ref<Probe> wrap{raw};
        CHECK(raw->GetStrongCount() == 2u);
        raw->Release();
    }
    CHECK(destroyed == 1);
}

TEST_CASE("MakeRef: returns a Ref with strong=1, weak=1")
{
    int destroyed = 0;
    {
        Ref<Probe> r = MakeRef<Probe>(&destroyed);
        REQUIRE(r);
        CHECK(r->GetStrongCount() == 1u);
        CHECK(r->GetWeakCount() == 1u);
        CHECK(r->payload_ == 42);
        CHECK(destroyed == 0);
    }
    CHECK(destroyed == 1);
}

TEST_CASE("Ref: copy ctor increments; move ctor steals without bumping")
{
    int d = 0;
    Ref<Probe> a = MakeRef<Probe>(&d);
    {
        Ref<Probe> b = a;
        CHECK(a->GetStrongCount() == 2u);
        CHECK(b.Get() == a.Get());
    }
    CHECK(a->GetStrongCount() == 1u);

    Ref<Probe> moved = std::move(a);
    CHECK(a.Get() == nullptr);
    CHECK(moved->GetStrongCount() == 1u);
}

TEST_CASE("Ref: copy and move assignment, including self-assignment")
{
    int d = 0;
    Ref<Probe> a = MakeRef<Probe>(&d);
    Ref<Probe> b;
    b = a;
    CHECK(a->GetStrongCount() == 2u);
    b = b;  // self-copy-assign — copy-and-swap handles it
    CHECK(a->GetStrongCount() == 2u);
    Ref<Probe> c;
    c = std::move(b);
    CHECK(b.Get() == nullptr);
    CHECK(a->GetStrongCount() == 2u);
    Ref<Probe> d2;
    d2 = nullptr;
    CHECK(d2.Get() == nullptr);
}

TEST_CASE("Ref: converting copy / move from Ref<Derived> to Ref<Base>")
{
    int d = 0;
    Ref<DerivedProbe> derived = MakeRef<DerivedProbe>(&d);
    Ref<Probe> base{derived};  // converting copy AddRefs
    CHECK(base.Get() == derived.Get());
    CHECK(derived->GetStrongCount() == 2u);

    Ref<Probe> moved{std::move(derived)};  // converting move Detaches
    CHECK(derived.Get() == nullptr);
    CHECK(moved->GetStrongCount() == 2u);
}

TEST_CASE("Ref: comparison operators compare the underlying pointer")
{
    int d = 0;
    Ref<Probe> a = MakeRef<Probe>(&d);
    Ref<Probe> b = a;
    Ref<Probe> c = MakeRef<Probe>(&d);
    CHECK(a == b);
    CHECK(a != c);
    CHECK_FALSE(a == c);
    CHECK(Ref<Probe>{} == nullptr);
    CHECK(a != nullptr);
    CHECK(nullptr != a);
}

TEST_CASE("Ref::Reset() and Reset(T*) and Detach/Attach")
{
    int d = 0;
    Ref<Probe> r = MakeRef<Probe>(&d);
    r.Reset();
    CHECK(r.Get() == nullptr);
    CHECK(d == 1);

    // Reset() on empty is a no-op.
    r.Reset();
    CHECK(r.Get() == nullptr);

    // Reset(T*) on empty replaces the pointer.
    r = MakeRef<Probe>(&d);
    Probe* raw = r.Detach();
    REQUIRE(raw != nullptr);
    CHECK(r.Get() == nullptr);
    CHECK(raw->GetStrongCount() == 1u);
    Ref<Probe> adopted;
    adopted.Attach(raw);
    CHECK(adopted.Get() == raw);
    CHECK(adopted->GetStrongCount() == 1u);
    CHECK(d == 1);  // adoption did not destroy

    // Attach releases the old held pointer (so Probe* raw is freed).
    Ref<Probe> swapTarget = MakeRef<Probe>(&d);
    swapTarget.Attach(nullptr);
    CHECK(swapTarget.Get() == nullptr);
}

TEST_CASE("Ref: Swap exchanges held pointers and updates ADL Swap")
{
    int d = 0;
    Ref<Probe> a = MakeRef<Probe>(&d);
    Ref<Probe> b = MakeRef<Probe>(&d);
    Probe* aRaw = a.Get();
    Probe* bRaw = b.Get();
    a.Swap(b);
    CHECK(a.Get() == bRaw);
    CHECK(b.Get() == aRaw);
    Swap(a, b);  // ADL free-function swap
    CHECK(a.Get() == aRaw);
}

TEST_CASE("Ref: arrow / star operators forward to the held object")
{
    Ref<Probe> r = MakeRef<Probe>();
    r->payload_ = 99;
    CHECK((*r).payload_ == 99);
}

// ---- WeakRef<T> ------------------------------------------------------------

TEST_CASE("WeakRef: default ctor is empty; Expired true; Lock returns null")
{
    WeakRef<Probe> w;
    CHECK(w.Expired());
    CHECK(w.Lock().Get() == nullptr);
    CHECK(w.GetUnsafe() == nullptr);
}

TEST_CASE("WeakRef: constructed from Ref<T>, Lock returns strong; Expired flips after strong drops")
{
    int destroyed = 0;
    WeakRef<Probe> weak;
    {
        Ref<Probe> strong = MakeRef<Probe>(&destroyed);
        weak = strong;
        CHECK_FALSE(weak.Expired());
        CHECK(strong->GetWeakCount() == 2u);  // 1 phantom + 1 weak

        Ref<Probe> locked = weak.Lock();
        CHECK(locked.Get() == strong.Get());
        CHECK(strong->GetStrongCount() == 2u);
    }
    CHECK(destroyed == 1);
    CHECK(weak.Expired());
    CHECK(weak.Lock().Get() == nullptr);
}

TEST_CASE("WeakRef: copy / move / nullptr assignment correctly counts")
{
    int d = 0;
    Ref<Probe> strong = MakeRef<Probe>(&d);
    WeakRef<Probe> a{strong};
    WeakRef<Probe> b{a};   // copy
    WeakRef<Probe> c{std::move(b)};  // move
    CHECK(b.GetUnsafe() == nullptr);
    CHECK(c.Lock().Get() == strong.Get());

    WeakRef<Probe> d2;
    d2 = a;
    CHECK(d2.Lock().Get() == strong.Get());
    d2 = nullptr;
    CHECK(d2.Expired());

    // Construct from raw T* (intrusive).
    WeakRef<Probe> direct{strong.Get()};
    CHECK(direct.Lock().Get() == strong.Get());
}

TEST_CASE("WeakRef: converting ctor from WeakRef<Derived>")
{
    int d = 0;
    Ref<DerivedProbe> strong = MakeRef<DerivedProbe>(&d);
    WeakRef<DerivedProbe> wd{strong};
    WeakRef<Probe> wb{wd};
    CHECK(wb.Lock().Get() == strong.Get());
}

TEST_CASE("WeakRef::Reset releases the weak ref")
{
    Ref<Probe> strong = MakeRef<Probe>();
    WeakRef<Probe> w{strong};
    CHECK(strong->GetWeakCount() == 2u);
    w.Reset();
    CHECK(w.Expired());
    CHECK(strong->GetWeakCount() == 1u);
}

TEST_CASE("Swap(WeakRef, WeakRef) exchanges control blocks")
{
    Ref<Probe> a = MakeRef<Probe>();
    Ref<Probe> b = MakeRef<Probe>();
    WeakRef<Probe> wa{a};
    WeakRef<Probe> wb{b};
    Swap(wa, wb);
    CHECK(wa.Lock().Get() == b.Get());
    CHECK(wb.Lock().Get() == a.Get());
}

// ---- EnableRefFromThis -----------------------------------------------------

TEST_CASE("EnableRefFromThis: RefFromThis returns a strong ref while alive")
{
    Ref<SharedProbe> owner = MakeRef<SharedProbe>();
    Ref<SharedProbe> alias = owner->RefFromThis();
    REQUIRE(alias);
    CHECK(alias.Get() == owner.Get());
    CHECK(owner->GetStrongCount() == 2u);
}

TEST_CASE("EnableRefFromThis: const overload returns Ref<const T>")
{
    Ref<SharedProbe> owner = MakeRef<SharedProbe>();
    const SharedProbe* p = owner.Get();
    Ref<const SharedProbe> alias = p->RefFromThis();
    REQUIRE(alias);
    CHECK(alias.Get() == owner.Get());
}

TEST_SUITE_END();
