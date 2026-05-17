/// @file
/// @brief Tests for Hylux reflection: TypeId / TypeName / TypeInfo / Field / Method / EnumInfo /
///        Any / TypeRegistry / ReflectionMacros.

#include "Core/Reflection/Reflection.h"

#include <doctest/doctest.h>

#include <array>
#include <ostream>
#include <span>
#include <string>
#include <utility>

using namespace Hylux;

TEST_SUITE_BEGIN("Core::Reflection");

// ---- Reflected fixtures (file scope so AutoRegister runs before main) -----

namespace
{

class ReflectableBase
{
    HYLUX_REFLECT_BODY(ReflectableBase)
public:
    virtual ~ReflectableBase() = default;
    int baseField{1};
    int Echo(int v) const { return v + 10; }
    void Inc() { ++baseField; }
};

class ReflectableDerived : public ReflectableBase
{
    HYLUX_REFLECT_BODY(ReflectableDerived)
public:
    int derivedField{0};
    int Add(int a, int b) const { return a + b; }
};

enum class ReflectedColor : std::uint16_t
{
    Red   = 1,
    Green = 2,
    Blue  = 3,
};

} // namespace

HYLUX_REFLECT_CLASS_BEGIN(ReflectableBase)
    HYLUX_REFLECT_FIELD(baseField)
    HYLUX_REFLECT_METHOD(Echo)
    HYLUX_REFLECT_METHOD(Inc)
HYLUX_REFLECT_CLASS_END(ReflectableBase)

HYLUX_REFLECT_CLASS_BEGIN(ReflectableDerived)
    HYLUX_REFLECT_BASE(ReflectableBase)
    HYLUX_REFLECT_FIELD_FLAGS(derivedField, ::Hylux::FieldFlags::ReadOnly)
    HYLUX_REFLECT_METHOD(Add)
HYLUX_REFLECT_CLASS_END(ReflectableDerived)

HYLUX_REFLECT_ENUM_BEGIN(ReflectedColor)
    HYLUX_REFLECT_ENUM_VALUE(Red)
    HYLUX_REFLECT_ENUM_VALUE(Green)
    HYLUX_REFLECT_ENUM_VALUE(Blue)
HYLUX_REFLECT_ENUM_END()

// ---- TypeId / TypeName ----------------------------------------------------

TEST_CASE("TypeId: invalid sentinel; ToU64; IsValid")
{
    static_assert(static_cast<std::uint64_t>(TypeId::Invalid) == 0);
    CHECK_FALSE(IsValid(TypeId::Invalid));
    CHECK(IsValid(TypeIdOf<int>()));
    CHECK(ToU64(TypeId::Invalid) == 0u);
}

TEST_CASE("TypeIdOf: stable per type; distinguishes cv-qualifiers and pointers")
{
    static_assert(TypeIdOf<int>() == TypeIdOf<int>());
    CHECK(TypeIdOf<int>() != TypeIdOf<float>());
    CHECK(TypeIdOf<int>() != TypeIdOf<const int>());  // TypeName preserves cv
    CHECK(TypeIdOf<int>() != TypeIdOf<int*>());
}

TEST_CASE("TypeName: strips MSVC 'class '/'struct '/'enum ' prefixes")
{
    constexpr auto baseName = TypeName<ReflectableBase>();
    const bool baseStartsWithClassKw = baseName.size() >= 6 &&
                                       baseName.substr(0, 6) == std::string_view{"class "};
    CHECK_FALSE(baseStartsWithClassKw);
    CHECK(baseName.find("ReflectableBase") != std::string_view::npos);

    constexpr auto enumName = TypeName<ReflectedColor>();
    const bool enumStartsWithEnumKw = enumName.size() >= 5 &&
                                      enumName.substr(0, 5) == std::string_view{"enum "};
    CHECK_FALSE(enumStartsWithEnumKw);
    CHECK(enumName.find("ReflectedColor") != std::string_view::npos);
}

// ---- TypeRegistry --------------------------------------------------------

TEST_CASE("TypeRegistry: AutoRegister populated ReflectableBase / ReflectableDerived / ReflectedColor")
{
    auto& reg = TypeRegistry::Get();
    const TypeInfo* base = reg.FindType(TypeIdOf<ReflectableBase>());
    const TypeInfo* derived = reg.FindType(TypeIdOf<ReflectableDerived>());
    const EnumInfo* color = reg.FindEnum(TypeIdOf<ReflectedColor>());
    REQUIRE(base != nullptr);
    REQUIRE(derived != nullptr);
    REQUIRE(color != nullptr);

    CHECK(reg.FindType(base->Name()) == base);
    CHECK(reg.FindEnum(color->Name()) == color);
}

TEST_CASE("TypeRegistry::FindType: unknown id / name returns nullptr")
{
    auto& reg = TypeRegistry::Get();
    CHECK(reg.FindType(TypeId::Invalid) == nullptr);
    CHECK(reg.FindType(std::string_view{"::Nonexistent::Type::That::Should::Not::Exist"}) == nullptr);
    CHECK(reg.FindEnum(TypeId::Invalid) == nullptr);
}

TEST_CASE("TypeRegistry::AllTypes / AllEnums: snapshot includes the reflected fixtures")
{
    auto& reg = TypeRegistry::Get();
    bool baseFound = false;
    for (const TypeInfo* t : reg.AllTypes())
    {
        if (t->Id() == TypeIdOf<ReflectableBase>()) baseFound = true;
    }
    CHECK(baseFound);

    bool colorFound = false;
    for (const EnumInfo* e : reg.AllEnums())
    {
        if (e->Id() == TypeIdOf<ReflectedColor>()) colorFound = true;
    }
    CHECK(colorFound);
}

// ---- TypeInfo / IsA / Construct / Cast ------------------------------------

TEST_CASE("TypeInfo: id/name/size/alignment from reflected type")
{
    const TypeInfo* derived = ReflectableDerived::GetStaticType();
    REQUIRE(derived != nullptr);
    CHECK(derived->Id() == TypeIdOf<ReflectableDerived>());
    CHECK(derived->Size() == sizeof(ReflectableDerived));
    CHECK(derived->Alignment() == alignof(ReflectableDerived));
    REQUIRE(derived->Base() != nullptr);
    CHECK(derived->Base()->Id() == TypeIdOf<ReflectableBase>());
}

TEST_CASE("TypeInfo::IsA: null, self, base, unrelated")
{
    const TypeInfo* base = ReflectableBase::GetStaticType();
    const TypeInfo* derived = ReflectableDerived::GetStaticType();
    REQUIRE(base != nullptr);
    REQUIRE(derived != nullptr);
    CHECK(base->IsA(base));
    CHECK(derived->IsA(base));
    CHECK_FALSE(base->IsA(derived));
    CHECK_FALSE(base->IsA(nullptr));
}

TEST_CASE("TypeInfo: NewInstance / Destroy roundtrip; trait queries")
{
    const TypeInfo* base = ReflectableBase::GetStaticType();
    REQUIRE(base != nullptr);
    CHECK(base->IsDefaultConstructible());
    CHECK(base->IsCopyConstructible());
    CHECK(base->IsMoveConstructible());

    void* raw = base->NewInstance();
    REQUIRE(raw != nullptr);
    base->Destroy(raw);

    base->Destroy(nullptr);  // no-op
}

TEST_CASE("TypeInfo::Construct<T>: typed construction for default-constructible T")
{
    const TypeInfo* derived = ReflectableDerived::GetStaticType();
    REQUIRE(derived != nullptr);
    ReflectableDerived* instance = derived->Construct<ReflectableDerived>();
    REQUIRE(instance != nullptr);
    CHECK(instance->baseField == 1);
    CHECK(instance->derivedField == 0);
    derived->Destroy(instance);
}

TEST_CASE("TypeInfo: CopyInstance / MoveInstance produce equivalent objects")
{
    const TypeInfo* base = ReflectableBase::GetStaticType();
    REQUIRE(base != nullptr);
    ReflectableBase* a = base->Construct<ReflectableBase>();
    a->baseField = 99;
    auto* copied = static_cast<ReflectableBase*>(base->CopyInstance(a));
    REQUIRE(copied != nullptr);
    CHECK(copied->baseField == 99);
    auto* moved = static_cast<ReflectableBase*>(base->MoveInstance(a));
    REQUIRE(moved != nullptr);
    CHECK(moved->baseField == 99);
    base->Destroy(a);
    base->Destroy(copied);
    base->Destroy(moved);
}

TEST_CASE("TypeInfo::FindField / FindMethod walk the base chain")
{
    const TypeInfo* derived = ReflectableDerived::GetStaticType();
    REQUIRE(derived != nullptr);

    const Field* baseFieldOnDerived = derived->FindField("baseField");
    REQUIRE(baseFieldOnDerived != nullptr);
    CHECK(baseFieldOnDerived->Offset() == offsetof(ReflectableBase, baseField));

    const Method* echo = derived->FindMethod("Echo");
    REQUIRE(echo != nullptr);
    CHECK(echo->Name() == "Echo");

    CHECK(derived->FindField("nope") == nullptr);
    CHECK(derived->FindMethod("nope") == nullptr);
}

TEST_CASE("Cast<T>: null, base->derived through reflected chain, unrelated returns null")
{
    ReflectableDerived d;
    ReflectableBase* basePtr = &d;
    CHECK(Cast<ReflectableDerived>(basePtr) == &d);
    CHECK(Cast<ReflectableBase>(basePtr) == &d);
    ReflectableBase* nullPtr = nullptr;
    CHECK(Cast<ReflectableDerived>(nullPtr) == nullptr);

    const ReflectableDerived cd;
    const ReflectableBase* cbasePtr = &cd;
    CHECK(Cast<const ReflectableDerived>(cbasePtr) == &cd);
}

// ---- Field ---------------------------------------------------------------

TEST_CASE("FieldFlags: bitwise ops + HasFlag")
{
    constexpr FieldFlags combined = FieldFlags::ReadOnly | FieldFlags::Serialize;
    static_assert(HasFlag(combined, FieldFlags::ReadOnly));
    static_assert(HasFlag(combined, FieldFlags::Serialize));
    static_assert(!HasFlag(combined, FieldFlags::EditorOnly));
    static_assert((combined & FieldFlags::ReadOnly) == FieldFlags::ReadOnly);
    static_assert(!HasFlag(FieldFlags::None, FieldFlags::None));
}

TEST_CASE("Field: GetPtr offsets / typed Get / Set / ReadOnly flag")
{
    const TypeInfo* base = ReflectableBase::GetStaticType();
    REQUIRE(base != nullptr);
    const Field* f = base->FindField("baseField");
    REQUIRE(f != nullptr);
    CHECK(f->Name() == "baseField");
    CHECK(f->Type() == TypeIdOf<int>());
    CHECK(f->Offset() == offsetof(ReflectableBase, baseField));
    CHECK(f->Flags() == FieldFlags::None);

    ReflectableBase obj;
    obj.baseField = 5;
    CHECK(f->Get<int>(&obj) == 5);
    f->Set<int>(&obj, 17);
    CHECK(obj.baseField == 17);

    const TypeInfo* derived = ReflectableDerived::GetStaticType();
    const Field* derivedF = derived->FindField("derivedField");
    REQUIRE(derivedF != nullptr);
    CHECK(HasFlag(derivedF->Flags(), FieldFlags::ReadOnly));
}

// ---- Method --------------------------------------------------------------

TEST_CASE("Method::Bind: returns ReturnType, ParamTypes; Invoke runs the member")
{
    const Method* echo = ReflectableBase::GetStaticType()->FindMethod("Echo");
    REQUIRE(echo != nullptr);
    CHECK(echo->ReturnType() == TypeIdOf<int>());
    CHECK(echo->Arity() == 1u);
    CHECK(echo->ParamTypes().size() == 1u);
    CHECK(echo->ParamTypes()[0] == TypeIdOf<int>());

    ReflectableBase obj;
    std::array<Any, 1> args = {Any::Make<int>(5)};
    Any result = echo->Invoke(&obj, std::span<Any>(args.data(), args.size()));
    CHECK(result.Type() == TypeIdOf<int>());
    CHECK(result.Get<int>() == 15);
}

TEST_CASE("Method::Invoke: void-returning method yields empty Any")
{
    const Method* inc = ReflectableBase::GetStaticType()->FindMethod("Inc");
    REQUIRE(inc != nullptr);
    CHECK(inc->Arity() == 0u);
    ReflectableBase obj;
    Any result = inc->Invoke(&obj, std::span<Any>{});
    CHECK(result.IsEmpty());
    CHECK(obj.baseField == 2);
}

TEST_CASE("Method::Invoke: two-arg const method via reflected Add")
{
    const Method* add = ReflectableDerived::GetStaticType()->FindMethod("Add");
    REQUIRE(add != nullptr);
    CHECK(add->Arity() == 2u);
    CHECK(add->ReturnType() == TypeIdOf<int>());

    ReflectableDerived obj;
    std::array<Any, 2> args = {Any::Make<int>(3), Any::Make<int>(4)};
    Any result = add->Invoke(&obj, std::span<Any>(args.data(), args.size()));
    CHECK(result.Get<int>() == 7);
}

// ---- EnumInfo ------------------------------------------------------------

TEST_CASE("EnumInfo: id / name / size / entries")
{
    const EnumInfo* color = TypeRegistry::Get().FindEnum(TypeIdOf<ReflectedColor>());
    REQUIRE(color != nullptr);
    CHECK(color->Id() == TypeIdOf<ReflectedColor>());
    CHECK(color->Size() == sizeof(ReflectedColor));
    CHECK(color->Entries().size() == 3u);
    CHECK(color->Entries()[0].name == "Red");
    CHECK(color->Entries()[0].value == 1);
}

TEST_CASE("EnumInfo: NameOf / ValueOf round-trip, missing returns nullopt")
{
    const EnumInfo* color = TypeRegistry::Get().FindEnum(TypeIdOf<ReflectedColor>());
    REQUIRE(color != nullptr);
    CHECK(color->NameOf(1) == std::string_view{"Red"});
    CHECK(color->NameOf(3) == std::string_view{"Blue"});
    CHECK_FALSE(color->NameOf(99).has_value());

    CHECK(color->ValueOf("Green") == 2);
    CHECK_FALSE(color->ValueOf("Yellow").has_value());
}

// ---- Any -----------------------------------------------------------------

TEST_CASE("Any: default state is empty")
{
    Any a;
    CHECK(a.IsEmpty());
    CHECK_FALSE(a.IsOwning());
    CHECK(a.RawPtr() == nullptr);
    CHECK(a.Type() == TypeId::Invalid);
    CHECK(a.TryGet<int>() == nullptr);
}

TEST_CASE("Any::Make: owning copy; Type matches; TryGet typed access")
{
    Any a = Any::Make<int>(7);
    CHECK_FALSE(a.IsEmpty());
    CHECK(a.IsOwning());
    CHECK(a.Type() == TypeIdOf<int>());
    REQUIRE(a.TryGet<int>() != nullptr);
    CHECK(*a.TryGet<int>() == 7);
    CHECK(a.Get<int>() == 7);
    CHECK(a.TryGet<float>() == nullptr);
}

TEST_CASE("Any: copy shares storage; destroying one does not free underlying value")
{
    Any a = Any::Make<int>(42);
    {
        Any b = a;
        CHECK(b.Get<int>() == 42);
    }
    CHECK(a.Get<int>() == 42);
}

TEST_CASE("Any::Borrow: non-owning, TryGet returns the pointer; cv stripped for TypeId")
{
    int value = 99;
    Any a = Any::Borrow<int>(&value);
    CHECK_FALSE(a.IsOwning());
    CHECK(a.Type() == TypeIdOf<int>());
    REQUIRE(a.TryGet<int>() == &value);

    const int constValue = 5;
    Any cb = Any::Borrow<const int>(&constValue);
    CHECK(cb.Type() == TypeIdOf<int>());  // remove_cv applied
}

TEST_SUITE_END();
