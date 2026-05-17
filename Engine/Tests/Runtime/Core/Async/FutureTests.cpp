/// @file
/// @brief Tests for Promise / Future and continuations.

#include "Core/Async/Future.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Core::Async");

#include <atomic>
#include <chrono>
#include <thread>

using namespace Hylux;

TEST_CASE("Future: default constructor is invalid")
{
    Future<int> f;
    CHECK_FALSE(f.IsValid());
    CHECK_FALSE(f.IsReady());
}

TEST_CASE("Future::MakeReady: already ready, Wait returns by reference, Then runs synchronously")
{
    Future<int> f = Future<int>::MakeReady(7);
    CHECK(f.IsValid());
    CHECK(f.IsReady());
    CHECK(f.Wait() == 7);
    // Multiple Waits return the same reference.
    CHECK(&f.Wait() == &f.Wait());

    int observed = 0;
    f.Then([&](int& v) { observed = v; });
    CHECK(observed == 7);
}

TEST_CASE("Future::MakeFailed: ready with default-constructed T")
{
    Future<int> f = Future<int>::MakeFailed();
    REQUIRE(f.IsReady());
    CHECK(f.Wait() == 0);
}

TEST_CASE("Promise/Future: continuation registered before Set fires after Set")
{
    Promise<int> p;
    Future<int> f = p.GetFuture();
    CHECK(f.IsValid());
    CHECK_FALSE(f.IsReady());

    int fired = 0;
    f.Then([&](int& v) { fired = v; });
    CHECK(fired == 0);

    p.Set(42);
    CHECK(fired == 42);
    CHECK(f.IsReady());
    CHECK(f.Wait() == 42);
}

TEST_CASE("Promise/Future: Wait blocks until Set is called from another thread")
{
    Promise<int> p;
    Future<int> f = p.GetFuture();

    std::thread setter([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        p.Set(123);
    });

    CHECK(f.Wait() == 123);
    setter.join();
}

TEST_CASE("Promise/Future: multiple continuations all fire once on Set")
{
    Promise<int> p;
    Future<int> f = p.GetFuture();
    std::atomic<int> count{0};
    f.Then([&](int&) { count.fetch_add(1); });
    f.Then([&](int&) { count.fetch_add(1); });
    f.Then([&](int&) { count.fetch_add(1); });
    p.Set(0);
    CHECK(count.load() == 3);
}

TEST_CASE("Promise: Wait returns a reference whose lifetime is tied to FutureState")
{
    Promise<int> p;
    Future<int> f = p.GetFuture();
    p.Set(99);
    int& ref = f.Wait();
    ref = 100;
    CHECK(f.Wait() == 100);
}

TEST_SUITE_END();
