/// @file
/// @brief Tests for Hylux::Future / Hylux::Promise.

#include "Core/Async/Future.h"

#include <doctest/doctest.h>

#include <atomic>
#include <chrono>
#include <thread>

using namespace Hylux;

TEST_CASE("MakeReady future is immediately ready and Wait returns the value")
{
    Future<int> f = Future<int>::MakeReady(42);
    REQUIRE(f.IsValid());
    CHECK(f.IsReady());
    CHECK(f.Wait() == 42);
}

TEST_CASE("MakeFailed yields a default-constructed value")
{
    Future<int> f = Future<int>::MakeFailed();
    REQUIRE(f.IsValid());
    CHECK(f.IsReady());
    CHECK(f.Wait() == 0);
}

TEST_CASE("Default-constructed future is not valid")
{
    Future<int> f;
    CHECK_FALSE(f.IsValid());
    CHECK_FALSE(f.IsReady());
}

TEST_CASE("Promise / Future round-trip set + wait")
{
    Promise<int> p;
    Future<int>  f = p.GetFuture();
    REQUIRE(f.IsValid());
    CHECK_FALSE(f.IsReady());
    p.Set(7);
    CHECK(f.IsReady());
    CHECK(f.Wait() == 7);
}

TEST_CASE("Then fires synchronously when the future is already ready")
{
    Future<int> f = Future<int>::MakeReady(100);
    int         observed = 0;
    f.Then([&observed](int& v) { observed = v; });
    CHECK(observed == 100);
}

TEST_CASE("Then registered before Set fires on Set")
{
    Promise<int> p;
    Future<int>  f = p.GetFuture();
    int          observed = 0;
    f.Then([&observed](int& v) { observed = v; });
    CHECK(observed == 0);
    p.Set(11);
    CHECK(observed == 11);
}

TEST_CASE("Then registered after Set fires synchronously")
{
    Promise<int> p;
    Future<int>  f = p.GetFuture();
    p.Set(33);
    int observed = 0;
    f.Then([&observed](int& v) { observed = v; });
    CHECK(observed == 33);
}

TEST_CASE("Multiple Then callbacks all fire and see the same value")
{
    Promise<int> p;
    Future<int>  f = p.GetFuture();
    int          a = 0;
    int          b = 0;
    int          c = 0;
    f.Then([&a](int& v) { a = v; });
    f.Then([&b](int& v) { b = v * 2; });
    f.Then([&c](int& v) { c = v * 3; });
    p.Set(5);
    CHECK(a == 5);
    CHECK(b == 10);
    CHECK(c == 15);
}

TEST_CASE("Cross-thread Wait blocks until Set is called")
{
    Promise<int>     p;
    Future<int>      f = p.GetFuture();
    std::atomic<int> observed{-1};

    std::thread waiter([&observed, f]() mutable {
        observed.store(f.Wait(), std::memory_order_release);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    CHECK(observed.load(std::memory_order_acquire) == -1);

    p.Set(99);
    waiter.join();
    CHECK(observed.load(std::memory_order_acquire) == 99);
}

TEST_CASE("Future is copyable and shares state across copies")
{
    Promise<int> p;
    Future<int>  a = p.GetFuture();
    Future<int>  b = a;
    REQUIRE(a.IsValid());
    REQUIRE(b.IsValid());
    p.Set(77);
    CHECK(a.IsReady());
    CHECK(b.IsReady());
    CHECK(a.Wait() == 77);
    CHECK(b.Wait() == 77);
}

namespace
{

struct MoveOnly
{
    int                              value{0};
    std::unique_ptr<int>             tag;
    MoveOnly() = default;
    explicit MoveOnly(int v) : value(v), tag(std::make_unique<int>(v)) {}
    MoveOnly(MoveOnly&&) noexcept            = default;
    MoveOnly& operator=(MoveOnly&&) noexcept = default;
    MoveOnly(const MoveOnly&)                = delete;
    MoveOnly& operator=(const MoveOnly&)     = delete;
};

} // namespace

TEST_CASE("Future supports move-only payloads")
{
    Future<MoveOnly> f = Future<MoveOnly>::MakeReady(MoveOnly{55});
    REQUIRE(f.IsReady());
    MoveOnly& payload = f.Wait();
    CHECK(payload.value == 55);
    REQUIRE(payload.tag);
    CHECK(*payload.tag == 55);
}
