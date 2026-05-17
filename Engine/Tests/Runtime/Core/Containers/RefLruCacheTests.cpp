/// @file
/// @brief Tests for Hylux::RefLruCache.

#include "Core/Containers/RefLruCache.h"

#include "Core/Memory/MakeRef.h"
#include "Core/Memory/Ref.h"
#include "Core/Memory/RefCounted.h"
#include "Core/Memory/WeakRef.h"

#include <doctest/doctest.h>

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
        if (destructions_ != nullptr)
        {
            ++(*destructions_);
        }
    }

    [[nodiscard]] std::uint64_t Bytes() const noexcept { return bytes_; }

private:
    std::uint64_t bytes_;
    int*          destructions_;
};

} // namespace

TEST_CASE("Insert + Get returns the same Ref and accounts the budget")
{
    RefLruCache<int, Block> cache(1024);
    Ref<Block>              b = MakeRef<Block>(100u);
    cache.Insert(1, b, b->Bytes());
    CHECK(cache.Size() == 1u);
    CHECK(cache.BytesInUse() == 100u);

    Ref<Block> got = cache.Get(1);
    REQUIRE(got);
    CHECK(got.Get() == b.Get());
}

TEST_CASE("Get on missing key returns null")
{
    RefLruCache<int, Block> cache(1024);
    CHECK_FALSE(cache.Get(42));
}

TEST_CASE("Eviction drops cache strong ref but keeps externally held value alive")
{
    int                     destroyed = 0;
    RefLruCache<int, Block> cache(150);
    Ref<Block>              keep = MakeRef<Block>(100u, &destroyed);

    cache.Insert(1, keep, keep->Bytes());
    CHECK(cache.BytesInUse() == 100u);

    Ref<Block> filler = MakeRef<Block>(100u);
    cache.Insert(2, filler, filler->Bytes());
    CHECK(cache.BytesInUse() == 100u);
    CHECK(destroyed == 0);

    Ref<Block> resurrected = cache.Get(1);
    REQUIRE(resurrected);
    CHECK(resurrected.Get() == keep.Get());
    CHECK(destroyed == 0);
    CHECK(cache.BytesInUse() == 100u);
}

TEST_CASE("Evicted slot whose external Ref is gone is erased on next Get")
{
    int                     destroyed = 0;
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

TEST_CASE("Get on a strong entry promotes it past other entries (LRU order)")
{
    int                     destroyed = 0;
    RefLruCache<int, Block> cache(300);

    {
        Ref<Block> a = MakeRef<Block>(100u, &destroyed);
        Ref<Block> b = MakeRef<Block>(100u, &destroyed);
        Ref<Block> c = MakeRef<Block>(100u, &destroyed);
        cache.Insert(1, a, 100);
        cache.Insert(2, b, 100);
        cache.Insert(3, c, 100);

        REQUIRE(cache.Get(1));
    }

    CHECK(destroyed == 0);

    Ref<Block> d = MakeRef<Block>(100u);
    cache.Insert(4, d, 100);

    CHECK(destroyed == 1);

    CHECK(cache.Get(1));
    CHECK_FALSE(cache.Get(2));
    CHECK(cache.Get(3));
    CHECK(cache.Get(4));
    CHECK(cache.BytesInUse() == 300u);
}

TEST_CASE("Insert replaces existing key and adjusts the budget")
{
    RefLruCache<int, Block> cache(500);
    Ref<Block>              a = MakeRef<Block>(100u);
    cache.Insert(1, a, 100);
    CHECK(cache.BytesInUse() == 100u);

    Ref<Block> b = MakeRef<Block>(250u);
    cache.Insert(1, b, 250);
    CHECK(cache.Size() == 1u);
    CHECK(cache.BytesInUse() == 250u);
    CHECK(cache.Get(1).Get() == b.Get());
}

TEST_CASE("Insert of null value is a no-op")
{
    RefLruCache<int, Block> cache(1024);
    cache.Insert(1, Ref<Block>{}, 100);
    CHECK(cache.Size() == 0u);
    CHECK(cache.BytesInUse() == 0u);
}

TEST_CASE("Clear drops every strong but resurrection still works")
{
    RefLruCache<int, Block> cache(500);
    Ref<Block>              a = MakeRef<Block>(100u);
    cache.Insert(1, a, 100);

    cache.Clear();
    CHECK(cache.BytesInUse() == 0u);
    CHECK(cache.Size() == 1u);

    Ref<Block> got = cache.Get(1);
    REQUIRE(got);
    CHECK(got.Get() == a.Get());
    CHECK(cache.BytesInUse() == 0u);

    cache.Insert(1, a, 100);
    CHECK(cache.BytesInUse() == 100u);
}

TEST_CASE("Over-budget single Insert evicts everything but keeps the new entry")
{
    RefLruCache<int, Block> cache(100);
    Ref<Block>              a = MakeRef<Block>(60u);
    cache.Insert(1, a, 60);
    Ref<Block> big = MakeRef<Block>(500u);
    cache.Insert(2, big, 500);

    CHECK(cache.Get(2));
    CHECK(cache.BytesInUse() == 500u);
}
