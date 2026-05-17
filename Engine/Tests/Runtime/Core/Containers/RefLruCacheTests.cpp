/// @file
/// @brief Tests for Hylux::RefLruCache.

#include "Core/Containers/RefLruCache.h"

#include "Core/Memory/MakeRef.h"
#include "Core/Memory/Ref.h"
#include "Core/Memory/RefCounted.h"
#include "Core/Memory/WeakRef.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("Core::Containers");

#include <cstdint>

using namespace Hylux;

namespace
{

class Block : public RefCounted
{
public:
    explicit Block(std::uint64_t bytes, int* destructions = nullptr) noexcept
        : bytes_(bytes), destructions_(destructions)
    {}
    ~Block() override
    {
        if (destructions_ != nullptr) ++(*destructions_);
    }
    [[nodiscard]] std::uint64_t Bytes() const noexcept { return bytes_; }

private:
    std::uint64_t bytes_;
    int*          destructions_;
};

} // namespace

TEST_CASE("RefLruCache: Insert + Get returns the same ref and tracks the budget")
{
    RefLruCache<int, Block> cache(1024);
    CHECK(cache.Budget() == 1024u);
    CHECK(cache.Size() == 0u);
    CHECK(cache.BytesInUse() == 0u);

    Ref<Block> b = MakeRef<Block>(100u);
    cache.Insert(1, b, 100u);
    CHECK(cache.Size() == 1u);
    CHECK(cache.BytesInUse() == 100u);

    Ref<Block> got = cache.Get(1);
    REQUIRE(got);
    CHECK(got.Get() == b.Get());
}

TEST_CASE("RefLruCache::Get on missing key returns null")
{
    RefLruCache<int, Block> cache(1024);
    CHECK_FALSE(cache.Get(42));
}

TEST_CASE("RefLruCache::Insert(null) is a no-op")
{
    RefLruCache<int, Block> cache(1024);
    cache.Insert(1, Ref<Block>{}, 100u);
    CHECK(cache.Size() == 0u);
    CHECK(cache.BytesInUse() == 0u);
}

TEST_CASE("RefLruCache: eviction drops the cache's strong, external ref keeps object alive")
{
    int destroyed = 0;
    RefLruCache<int, Block> cache(150);
    Ref<Block> keep = MakeRef<Block>(100u, &destroyed);

    cache.Insert(1, keep, keep->Bytes());
    CHECK(cache.BytesInUse() == 100u);

    Ref<Block> filler = MakeRef<Block>(100u);
    cache.Insert(2, filler, filler->Bytes());
    CHECK(cache.BytesInUse() == 100u);
    CHECK(destroyed == 0);

    // Resurrect entry 1 via its weak ref.
    Ref<Block> resurrected = cache.Get(1);
    REQUIRE(resurrected);
    CHECK(resurrected.Get() == keep.Get());
    CHECK(destroyed == 0);
    CHECK(cache.BytesInUse() == 100u);
}

TEST_CASE("RefLruCache: evicted slot with expired weak is erased on next Get")
{
    int destroyed = 0;
    RefLruCache<int, Block> cache(150);

    {
        Ref<Block> tmp = MakeRef<Block>(100u, &destroyed);
        cache.Insert(1, tmp, tmp->Bytes());
        Ref<Block> filler = MakeRef<Block>(100u);
        cache.Insert(2, filler, filler->Bytes());
        CHECK(destroyed == 0);
    }

    CHECK(destroyed == 1);
    CHECK(cache.Size() == 2u);
    CHECK_FALSE(cache.Get(1));
    CHECK(cache.Size() == 1u);
}

TEST_CASE("RefLruCache: Get on a live strong promotes the entry (LRU order)")
{
    int destroyed = 0;
    RefLruCache<int, Block> cache(300);

    {
        Ref<Block> a = MakeRef<Block>(100u, &destroyed);
        Ref<Block> b = MakeRef<Block>(100u, &destroyed);
        Ref<Block> c = MakeRef<Block>(100u, &destroyed);
        cache.Insert(1, a, 100);
        cache.Insert(2, b, 100);
        cache.Insert(3, c, 100);
        REQUIRE(cache.Get(1));  // promotes 1 past 2 and 3
    }
    CHECK(destroyed == 0);

    Ref<Block> d = MakeRef<Block>(100u);
    cache.Insert(4, d, 100);   // forces eviction of LRU (now 2)
    CHECK(destroyed == 1);

    CHECK(cache.Get(1));
    CHECK_FALSE(cache.Get(2));
    CHECK(cache.Get(3));
    CHECK(cache.Get(4));
    CHECK(cache.BytesInUse() == 300u);
}

TEST_CASE("RefLruCache::Insert replaces existing key and adjusts the budget")
{
    RefLruCache<int, Block> cache(500);
    Ref<Block> a = MakeRef<Block>(100u);
    cache.Insert(1, a, 100);
    CHECK(cache.BytesInUse() == 100u);

    Ref<Block> b = MakeRef<Block>(250u);
    cache.Insert(1, b, 250);
    CHECK(cache.Size() == 1u);
    CHECK(cache.BytesInUse() == 250u);
    CHECK(cache.Get(1).Get() == b.Get());
}

TEST_CASE("RefLruCache::Clear: drops every strong but weak resurrection still works")
{
    RefLruCache<int, Block> cache(500);
    Ref<Block> a = MakeRef<Block>(100u);
    cache.Insert(1, a, 100);

    cache.Clear();
    CHECK(cache.BytesInUse() == 0u);

    Ref<Block> got = cache.Get(1);
    REQUIRE(got);
    CHECK(got.Get() == a.Get());
    CHECK(cache.BytesInUse() == 0u);

    cache.Insert(1, a, 100);
    CHECK(cache.BytesInUse() == 100u);
}

TEST_CASE("RefLruCache: over-budget Insert evicts everything and still admits the new entry")
{
    RefLruCache<int, Block> cache(100);
    Ref<Block> a = MakeRef<Block>(60u);
    cache.Insert(1, a, 60);
    Ref<Block> big = MakeRef<Block>(500u);
    cache.Insert(2, big, 500);

    CHECK(cache.Get(2));
    CHECK(cache.BytesInUse() == 500u);
}

TEST_CASE("RefLruCache: zero-budget cache still admits the first entry")
{
    RefLruCache<int, Block> cache(0);
    Ref<Block> a = MakeRef<Block>(10u);
    cache.Insert(1, a, 10);
    CHECK(cache.Get(1));
    CHECK(cache.BytesInUse() == 10u);
}

TEST_SUITE_END();
