/// @file
/// @brief Tests for Hylux::SmallVector.

#include "Core/Containers/SmallVector.h"

#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <utility>

TEST_SUITE_BEGIN("Core::Containers");

using namespace Hylux;

namespace
{

struct Counted
{
    static int liveCount;
    int        value{0};

    Counted() noexcept { ++liveCount; }
    explicit Counted(int v) noexcept : value(v) { ++liveCount; }
    Counted(const Counted& other) noexcept : value(other.value) { ++liveCount; }
    Counted(Counted&& other) noexcept : value(other.value) { other.value = 0; ++liveCount; }
    Counted& operator=(const Counted& other) noexcept { value = other.value; return *this; }
    Counted& operator=(Counted&& other) noexcept { value = other.value; other.value = 0; return *this; }
    ~Counted() { --liveCount; }
};

int Counted::liveCount = 0;

} // namespace

TEST_CASE("SmallVector: default constructed is empty and inline")
{
    SmallVector<int, 4> v;
    CHECK(v.size() == 0u);
    CHECK(v.capacity() == 4u);
    CHECK(v.empty());
    CHECK(v.IsInline());
}

TEST_CASE("SmallVector: push_back stays inline up to N")
{
    SmallVector<int, 4> v;
    v.PushBack(10);
    v.PushBack(20);
    v.PushBack(30);
    v.PushBack(40);
    CHECK(v.size() == 4u);
    CHECK(v.capacity() == 4u);
    CHECK(v.IsInline());
    CHECK(v[0] == 10);
    CHECK(v[3] == 40);
}

TEST_CASE("SmallVector: push_back beyond N promotes to heap and preserves elements")
{
    SmallVector<int, 2> v;
    v.PushBack(1);
    v.PushBack(2);
    CHECK(v.IsInline());
    v.PushBack(3);
    CHECK_FALSE(v.IsInline());
    CHECK(v.capacity() >= 3u);
    CHECK(v.size() == 3u);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
    CHECK(v[2] == 3);
}

TEST_CASE("SmallVector: EmplaceBack returns reference and constructs in place")
{
    SmallVector<std::string, 2> v;
    auto& ref = v.EmplaceBack(5, 'x');
    CHECK(ref == "xxxxx");
    CHECK(&ref == &v[0]);
}

TEST_CASE("SmallVector: range-based for and iterators traverse elements")
{
    SmallVector<int, 4> v{1, 2, 3, 4, 5}; // promotes to heap
    int sum = 0;
    for (int x : v) sum += x;
    CHECK(sum == 15);
    CHECK(*v.begin() == 1);
    CHECK(*(v.end() - 1) == 5);
}

TEST_CASE("SmallVector: copy constructor duplicates contents")
{
    SmallVector<int, 2> a{1, 2, 3};
    SmallVector<int, 2> b(a);
    CHECK(b.size() == 3u);
    CHECK(b[0] == 1);
    CHECK(b[2] == 3);
    b.PushBack(4);
    CHECK(a.size() == 3u);
    CHECK(b.size() == 4u);
}

TEST_CASE("SmallVector: move constructor steals heap pointer")
{
    SmallVector<int, 2> a{10, 20, 30, 40};
    REQUIRE_FALSE(a.IsInline());
    const int* originalData = a.data();

    SmallVector<int, 2> b(std::move(a));
    CHECK(b.size() == 4u);
    CHECK(b.data() == originalData);
    CHECK(a.size() == 0u);
    CHECK(a.IsInline());
}

TEST_CASE("SmallVector: move constructor copies elements when source is inline")
{
    SmallVector<int, 4> a{1, 2, 3};
    REQUIRE(a.IsInline());
    SmallVector<int, 4> b(std::move(a));
    CHECK(b.size() == 3u);
    CHECK(b[0] == 1);
    CHECK(b[2] == 3);
    CHECK(a.size() == 0u);
}

TEST_CASE("SmallVector: copy assignment replaces contents and releases prior heap")
{
    SmallVector<int, 2> a{1, 2, 3, 4};   // heap
    SmallVector<int, 2> b{99};
    b = a;
    CHECK(b.size() == 4u);
    CHECK(b[0] == 1);
    CHECK(b[3] == 4);
}

TEST_CASE("SmallVector: move assignment releases prior storage")
{
    SmallVector<int, 2> a{1, 2, 3, 4, 5};
    SmallVector<int, 2> b{7, 8, 9};
    b = std::move(a);
    CHECK(b.size() == 5u);
    CHECK(b[4] == 5);
    CHECK(a.empty());
}

TEST_CASE("SmallVector: PopBack destroys the last element")
{
    Counted::liveCount = 0;
    {
        SmallVector<Counted, 4> v;
        v.EmplaceBack(1);
        v.EmplaceBack(2);
        v.EmplaceBack(3);
        CHECK(Counted::liveCount == 3);
        v.PopBack();
        CHECK(Counted::liveCount == 2);
        CHECK(v.back().value == 2);
    }
    CHECK(Counted::liveCount == 0);
}

TEST_CASE("SmallVector: Clear destroys every element but keeps capacity")
{
    Counted::liveCount = 0;
    SmallVector<Counted, 2> v;
    v.EmplaceBack(1);
    v.EmplaceBack(2);
    v.EmplaceBack(3); // promotes
    const std::size_t capacityBefore = v.capacity();
    v.Clear();
    CHECK(v.empty());
    CHECK(Counted::liveCount == 0);
    CHECK(v.capacity() == capacityBefore);
}

TEST_CASE("SmallVector: Resize grows with default-constructed values and shrinks via destruction")
{
    Counted::liveCount = 0;
    SmallVector<Counted, 4> v;
    v.Resize(3);
    CHECK(v.size() == 3u);
    CHECK(Counted::liveCount == 3);
    v.Resize(1);
    CHECK(v.size() == 1u);
    CHECK(Counted::liveCount == 1);
}

TEST_CASE("SmallVector: Erase single element shifts the tail down")
{
    SmallVector<int, 4> v{1, 2, 3, 4, 5};
    auto it = v.Erase(v.begin() + 2);
    CHECK(v.size() == 4u);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
    CHECK(v[2] == 4);
    CHECK(v[3] == 5);
    CHECK(*it == 4);
}

TEST_CASE("SmallVector: Erase range removes [first, last)")
{
    SmallVector<int, 4> v{1, 2, 3, 4, 5};
    v.Erase(v.begin() + 1, v.begin() + 4);
    CHECK(v.size() == 2u);
    CHECK(v[0] == 1);
    CHECK(v[1] == 5);
}

TEST_CASE("SmallVector: Reserve allocates heap exactly once when crossing N")
{
    SmallVector<int, 4> v;
    v.Reserve(16);
    CHECK(v.capacity() >= 16u);
    CHECK_FALSE(v.IsInline());
    for (int i = 0; i < 16; ++i) v.PushBack(i);
    CHECK(v.capacity() >= 16u);
    CHECK(v[15] == 15);
}

TEST_CASE("SmallVector: ShrinkToFit returns to inline when size <= N")
{
    SmallVector<int, 4> v{1, 2, 3, 4, 5}; // heap
    REQUIRE_FALSE(v.IsInline());
    v.PopBack();
    v.ShrinkToFit();
    CHECK(v.size() == 4u);
    CHECK(v.IsInline());
    CHECK(v.capacity() == 4u);
    CHECK(v[0] == 1);
    CHECK(v[3] == 4);
}

TEST_CASE("SmallVector: holds move-only types")
{
    SmallVector<std::unique_ptr<int>, 2> v;
    v.EmplaceBack(std::make_unique<int>(7));
    v.EmplaceBack(std::make_unique<int>(11));
    v.EmplaceBack(std::make_unique<int>(13)); // forces promotion
    REQUIRE(v.size() == 3u);
    CHECK(*v[0] == 7);
    CHECK(*v[1] == 11);
    CHECK(*v[2] == 13);
}

TEST_SUITE_END();
