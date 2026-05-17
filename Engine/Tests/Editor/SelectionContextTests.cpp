/// @file
/// @brief Tests for SelectionContext signal emission and dedup behaviour.

#include "Core/Memory/MakeRef.h"
#include "Selection/ISelectable.h"
#include "Selection/SelectionContext.h"

#include <doctest/doctest.h>

#include <QObject>
#include <QSignalSpy>

using namespace Hylux;
using namespace Hylux::Editor;

namespace
{

class TestSelectable final : public ISelectable
{
public:
    explicit TestSelectable(SelectionId id, std::string_view name) : id_(id), name_(name) {}

    [[nodiscard]] SelectionId      GetSelectionId() const override { return id_; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return name_; }

private:
    SelectionId id_;
    std::string name_;
};

[[nodiscard]] Ref<TestSelectable> MakeOne(std::uint64_t handle, std::string_view name = "test")
{
    return MakeRef<TestSelectable>(SelectionId{TypeIdOf<TestSelectable>(), handle}, name);
}

} // namespace

TEST_CASE("SelectionContext Select emits exactly once")
{
    SelectionContext ctx;
    QSignalSpy       spy(&ctx, &SelectionContext::selectionChanged);
    ctx.Select(MakeOne(1));
    CHECK(spy.count() == 1);
    CHECK(ctx.Size() == 1);
}

TEST_CASE("SelectionContext Select with same id is a no-op")
{
    SelectionContext ctx;
    ctx.Select(MakeOne(7));
    QSignalSpy spy(&ctx, &SelectionContext::selectionChanged);
    ctx.Select(MakeOne(7));
    CHECK(spy.count() == 0);
}

TEST_CASE("SelectionContext AddToSelection extends the set")
{
    SelectionContext ctx;
    ctx.Select(MakeOne(1, "a"));
    QSignalSpy spy(&ctx, &SelectionContext::selectionChanged);
    ctx.AddToSelection(MakeOne(2, "b"));
    CHECK(spy.count() == 1);
    CHECK(ctx.Size() == 2);
    ctx.AddToSelection(MakeOne(2, "b"));
    CHECK(spy.count() == 1);
}

TEST_CASE("SelectionContext Clear emits only when non-empty")
{
    SelectionContext ctx;
    QSignalSpy       spy(&ctx, &SelectionContext::selectionChanged);
    ctx.Clear();
    CHECK(spy.count() == 0);
    ctx.Select(MakeOne(9));
    spy.clear();
    ctx.Clear();
    CHECK(spy.count() == 1);
}
