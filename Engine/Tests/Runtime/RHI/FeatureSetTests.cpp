/// @file
/// @brief Tests for the FeatureSet bitset wrapper.

#include "RHI/RHIFeature.h"

#include <doctest/doctest.h>

#include <ostream>  // stringify std::string_view for doctest CHECKs
TEST_SUITE_BEGIN("RHI");

using namespace Hylux::RHI;

TEST_CASE("FeatureSet: default is empty")
{
    FeatureSet f;
    CHECK(f.Empty());
    CHECK(f.Count() == 0u);
    CHECK_FALSE(f.Has(Feature::Bindless));
}

TEST_CASE("FeatureSet: Set / Clear / Has")
{
    FeatureSet f;
    f.Set(Feature::Bindless);
    f.Set(Feature::DynamicRendering);
    CHECK(f.Has(Feature::Bindless));
    CHECK(f.Has(Feature::DynamicRendering));
    CHECK_FALSE(f.Has(Feature::RayTracing));
    CHECK(f.Count() == 2u);

    f.Clear(Feature::Bindless);
    CHECK_FALSE(f.Has(Feature::Bindless));
    CHECK(f.Count() == 1u);
}

TEST_CASE("FeatureSet: initializer-list ctor populates flags")
{
    FeatureSet f{Feature::MeshShader, Feature::RayQuery, Feature::CaptureNsight};
    CHECK(f.Has(Feature::MeshShader));
    CHECK(f.Has(Feature::RayQuery));
    CHECK(f.Has(Feature::CaptureNsight));
    CHECK(f.Count() == 3u);
}

TEST_CASE("FeatureSet: union and intersection operators")
{
    const FeatureSet a{Feature::Bindless, Feature::RayTracing};
    const FeatureSet b{Feature::Bindless, Feature::MeshShader};

    const FeatureSet unionSet = a | b;
    CHECK(unionSet.Has(Feature::Bindless));
    CHECK(unionSet.Has(Feature::RayTracing));
    CHECK(unionSet.Has(Feature::MeshShader));
    CHECK(unionSet.Count() == 3u);

    const FeatureSet intersection = a & b;
    CHECK(intersection.Has(Feature::Bindless));
    CHECK_FALSE(intersection.Has(Feature::RayTracing));
    CHECK_FALSE(intersection.Has(Feature::MeshShader));
    CHECK(intersection.Count() == 1u);
}

TEST_CASE("FeatureSet: compound assignment + equality")
{
    FeatureSet a{Feature::Bindless};
    FeatureSet b{Feature::Bindless, Feature::RayTracing};
    a |= b;
    CHECK(a == b);

    a &= FeatureSet{Feature::Bindless};
    CHECK(a == FeatureSet{Feature::Bindless});
}

TEST_CASE("FeatureSet: kFeatureSetBits accommodates Feature::Count")
{
    static_assert(static_cast<std::size_t>(Feature::Count) <= kFeatureSetBits);
}

TEST_SUITE_END();
