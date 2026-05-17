/// @file
/// @brief Tests for the Hylux reflection subsystem: TypeRegistry, TypeInfo, Field,
///        Method, EnumInfo, Any, and the HYLUX_REFLECT_* macros.

#include "Core/Reflection/Reflection.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <span>
#include <string_view>

namespace HyluxReflectionTests
{

enum class Mood : std::int32_t
{
    Sad   = 0,
    Meh   = 1,
    Happy = 2,
};

/// @brief Polymorphic reflected base used by the tests below.
class Animal
{
    HYLUX_REFLECT_BODY(Animal)
public:
    Animal() = default;
    virtual ~Animal() = default;

    int legs_{4};

    int Bark(int times) { return legs_ * times; }
    [[nodiscard]] int Counted() const { return legs_; }
};

/// @brief Reflected subclass — used to exercise the base chain and Cast<>.
class Dog : public Animal
{
    HYLUX_REFLECT_BODY(Dog)
public:
    Dog() = default;

    float happiness_{0.5f};
};

/// @brief Reflected type with no methods, used to exercise field-only registration.
class JustFields
{
    HYLUX_REFLECT_BODY_POD(JustFields)
public:
    int a_{1};
    int b_{2};
};

} // namespace HyluxReflectionTests

HYLUX_REFLECT_CLASS_BEGIN(HyluxReflectionTests::Animal)
HYLUX_REFLECT_FIELD(legs_)
HYLUX_REFLECT_METHOD(Bark)
HYLUX_REFLECT_METHOD(Counted)
HYLUX_REFLECT_CLASS_END(HyluxReflectionTests::Animal)

HYLUX_REFLECT_CLASS_BEGIN(HyluxReflectionTests::Dog)
HYLUX_REFLECT_BASE(HyluxReflectionTests::Animal)
HYLUX_REFLECT_FIELD(happiness_)
HYLUX_REFLECT_CLASS_END(HyluxReflectionTests::Dog)

HYLUX_REFLECT_CLASS_BEGIN(HyluxReflectionTests::JustFields)
HYLUX_REFLECT_FIELD(a_)
HYLUX_REFLECT_FIELD_FLAGS(b_, ::Hylux::FieldFlags::ReadOnly | ::Hylux::FieldFlags::Serialize)
HYLUX_REFLECT_CLASS_END(HyluxReflectionTests::JustFields)

HYLUX_REFLECT_ENUM_BEGIN(HyluxReflectionTests::Mood)
HYLUX_REFLECT_ENUM_VALUE(Sad)
HYLUX_REFLECT_ENUM_VALUE(Meh)
HYLUX_REFLECT_ENUM_VALUE(Happy)
HYLUX_REFLECT_ENUM_END()

using namespace Hylux;
using HyluxReflectionTests::Animal;
using HyluxReflectionTests::Dog;
using HyluxReflectionTests::JustFields;
using HyluxReflectionTests::Mood;

// ---- TypeId / TypeName ------------------------------------------------------

TEST_CASE("TypeIdOf is stable, non-zero, and distinct for distinct types")
{
    constexpr TypeId idAnimal = TypeIdOf<Animal>();
    constexpr TypeId idDog    = TypeIdOf<Dog>();
    static_assert(IsValid(idAnimal));
    static_assert(IsValid(idDog));
    static_assert(idAnimal != idDog);
    CHECK(IsValid(idAnimal));
    CHECK(idAnimal != idDog);
    CHECK(ToU64(TypeId::Invalid) == 0u);
}

TEST_CASE("TypeName returns a non-empty string and strips the class keyword")
{
    constexpr std::string_view name = TypeName<Animal>();
    CHECK_FALSE(name.empty());
    CHECK(name.find("class ") == std::string_view::npos);
    CHECK(name.find("Animal") != std::string_view::npos);
}

// ---- TypeRegistry / TypeInfo ------------------------------------------------

TEST_CASE("Reflected types appear in the global registry under their TypeId")
{
    const TypeInfo* animal = TypeRegistry::Get().FindType(TypeIdOf<Animal>());
    REQUIRE(animal != nullptr);
    CHECK(animal->Id() == TypeIdOf<Animal>());
    CHECK(animal->Size() == sizeof(Animal));
    CHECK(animal->Alignment() == alignof(Animal));
    CHECK(animal->Base() == nullptr);
}

TEST_CASE("Reflected types are also discoverable by canonical name")
{
    const TypeInfo* byId   = TypeRegistry::Get().FindType(TypeIdOf<Animal>());
    const TypeInfo* byName = TypeRegistry::Get().FindType(TypeName<Animal>());
    REQUIRE(byId != nullptr);
    REQUIRE(byName != nullptr);
    CHECK(byId == byName);
}

TEST_CASE("Animal::GetStaticType returns the registered TypeInfo")
{
    const TypeInfo* fromMacro = Animal::GetStaticType();
    REQUIRE(fromMacro != nullptr);
    CHECK(fromMacro == TypeRegistry::Get().FindType(TypeIdOf<Animal>()));
}

TEST_CASE("GetType on an instance dispatches virtually to the most-derived type")
{
    Dog dog;
    Animal* asBase = &dog;
    REQUIRE(asBase->GetType() != nullptr);
    CHECK(asBase->GetType() == Dog::GetStaticType());
}

TEST_CASE("TypeInfo::IsA walks the reflected base chain")
{
    const TypeInfo* dog    = Dog::GetStaticType();
    const TypeInfo* animal = Animal::GetStaticType();
    REQUIRE(dog != nullptr);
    REQUIRE(animal != nullptr);
    CHECK(dog->IsA(dog));
    CHECK(dog->IsA(animal));
    CHECK_FALSE(animal->IsA(dog));
    CHECK_FALSE(dog->IsA(nullptr));
}

TEST_CASE("Cast<T>() returns a typed pointer for a matching base chain")
{
    Dog dog;
    Animal* base = &dog;
    Dog* roundTrip = Cast<Dog>(base);
    CHECK(roundTrip == &dog);

    Animal animalOnly;
    Animal* basePtr = &animalOnly;
    CHECK(Cast<Dog>(basePtr) == nullptr);
}

TEST_CASE("TypeInfo construction/destruction lifecycle round-trips")
{
    const TypeInfo* dog = Dog::GetStaticType();
    REQUIRE(dog != nullptr);
    REQUIRE(dog->IsDefaultConstructible());

    Dog* instance = dog->Construct<Dog>();
    REQUIRE(instance != nullptr);
    CHECK(instance->legs_ == 4);
    dog->Destroy(instance);
}

TEST_CASE("TypeInfo can copy-construct a reflected instance")
{
    const TypeInfo* animal = Animal::GetStaticType();
    REQUIRE(animal->IsCopyConstructible());

    Animal source;
    source.legs_ = 11;
    auto* copy = static_cast<Animal*>(animal->CopyInstance(&source));
    REQUIRE(copy != nullptr);
    CHECK(copy->legs_ == 11);
    animal->Destroy(copy);
}

// ---- Field -------------------------------------------------------------------

TEST_CASE("Field exposes name, offset, type id, and value access")
{
    const TypeInfo* animal = Animal::GetStaticType();
    const Field* legs = animal->FindField("legs_");
    REQUIRE(legs != nullptr);
    CHECK(legs->Name() == "legs_");
    CHECK(legs->Type() == TypeIdOf<int>());
    CHECK(legs->Offset() == offsetof(Animal, legs_));

    Animal instance;
    legs->Set<int>(&instance, 99);
    CHECK(legs->Get<int>(&instance) == 99);
    CHECK(instance.legs_ == 99);
}

TEST_CASE("FindField walks the base chain to resolve inherited members")
{
    const TypeInfo* dog = Dog::GetStaticType();
    const Field* inherited = dog->FindField("legs_");
    const Field* own       = dog->FindField("happiness_");
    REQUIRE(inherited != nullptr);
    REQUIRE(own != nullptr);
    CHECK(inherited->Type() == TypeIdOf<int>());
    CHECK(own->Type() == TypeIdOf<float>());
}

TEST_CASE("FieldFlags compose bitwise and HasFlag detects membership")
{
    const FieldFlags combined = FieldFlags::ReadOnly | FieldFlags::Serialize;
    CHECK(HasFlag(combined, FieldFlags::ReadOnly));
    CHECK(HasFlag(combined, FieldFlags::Serialize));
    CHECK_FALSE(HasFlag(combined, FieldFlags::Transient));

    const TypeInfo* podType = JustFields::GetStaticType();
    REQUIRE(podType != nullptr);
    const Field* b = podType->FindField("b_");
    REQUIRE(b != nullptr);
    CHECK(HasFlag(b->Flags(), FieldFlags::ReadOnly));
    CHECK(HasFlag(b->Flags(), FieldFlags::Serialize));
}

// ---- Method ------------------------------------------------------------------

TEST_CASE("Method records return type, parameter types, and arity")
{
    const TypeInfo* animal = Animal::GetStaticType();
    const Method* bark = animal->FindMethod("Bark");
    REQUIRE(bark != nullptr);
    CHECK(bark->Name() == "Bark");
    CHECK(bark->Arity() == 1u);
    CHECK(bark->ReturnType() == TypeIdOf<int>());
    REQUIRE(bark->ParamTypes().size() == 1u);
    CHECK(bark->ParamTypes()[0] == TypeIdOf<int>());
}

TEST_CASE("Method::Invoke runs a non-const member with Any-wrapped args")
{
    const TypeInfo* animal = Animal::GetStaticType();
    const Method* bark = animal->FindMethod("Bark");
    REQUIRE(bark != nullptr);

    Animal instance;
    instance.legs_ = 4;
    Any times = Any::Make<int>(3);
    Any args[] = {times};
    Any result = bark->Invoke(&instance, std::span<Any>(args));
    CHECK(result.Type() == TypeIdOf<int>());
    CHECK(result.Get<int>() == 12);
}

TEST_CASE("Method::Invoke runs a const member with no args")
{
    const TypeInfo* animal = Animal::GetStaticType();
    const Method* counted = animal->FindMethod("Counted");
    REQUIRE(counted != nullptr);
    CHECK(counted->Arity() == 0u);

    Animal instance;
    instance.legs_ = 7;
    Any result = counted->Invoke(&instance, std::span<Any>{});
    CHECK(result.Get<int>() == 7);
}

// ---- Any ---------------------------------------------------------------------

TEST_CASE("Default Any is empty")
{
    Any empty;
    CHECK(empty.IsEmpty());
    CHECK(empty.Type() == TypeId::Invalid);
    CHECK(empty.RawPtr() == nullptr);
    CHECK(empty.TryGet<int>() == nullptr);
}

TEST_CASE("Any::Make owns a copy and exposes the value via Get / TryGet")
{
    Any boxed = Any::Make<int>(42);
    CHECK_FALSE(boxed.IsEmpty());
    CHECK(boxed.IsOwning());
    CHECK(boxed.Type() == TypeIdOf<int>());
    REQUIRE(boxed.TryGet<int>() != nullptr);
    CHECK(*boxed.TryGet<int>() == 42);
    CHECK(boxed.Get<int>() == 42);
    CHECK(boxed.TryGet<float>() == nullptr);
}

TEST_CASE("Any::Borrow aliases an external value without owning it")
{
    int value = 99;
    Any borrowed = Any::Borrow(&value);
    CHECK_FALSE(borrowed.IsOwning());
    CHECK(borrowed.Type() == TypeIdOf<int>());
    CHECK(borrowed.RawPtr() == &value);

    borrowed.Get<int>() = 123;
    CHECK(value == 123);
}

// ---- EnumInfo ----------------------------------------------------------------

TEST_CASE("Enum registration captures all enumerators in both directions")
{
    const EnumInfo* mood = TypeRegistry::Get().FindEnum(TypeIdOf<Mood>());
    REQUIRE(mood != nullptr);
    CHECK(mood->Size() == sizeof(Mood));
    CHECK(mood->Entries().size() == 3u);

    CHECK(mood->NameOf(static_cast<std::int64_t>(Mood::Sad))   == "Sad");
    CHECK(mood->NameOf(static_cast<std::int64_t>(Mood::Meh))   == "Meh");
    CHECK(mood->NameOf(static_cast<std::int64_t>(Mood::Happy)) == "Happy");
    CHECK_FALSE(mood->NameOf(999).has_value());

    CHECK(mood->ValueOf("Happy") == static_cast<std::int64_t>(Mood::Happy));
    CHECK(mood->ValueOf("Sad")   == static_cast<std::int64_t>(Mood::Sad));
    CHECK_FALSE(mood->ValueOf("Furious").has_value());
}

TEST_CASE("Enums are also findable by canonical name")
{
    const EnumInfo* byId   = TypeRegistry::Get().FindEnum(TypeIdOf<Mood>());
    const EnumInfo* byName = TypeRegistry::Get().FindEnum(TypeName<Mood>());
    REQUIRE(byId != nullptr);
    REQUIRE(byName != nullptr);
    CHECK(byId == byName);
}
