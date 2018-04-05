#include <cassert>
#include <cinttypes>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

// 1. Type as values

template <typename T>
struct Type {};

// disallow recursive types
template <typename T>
struct Type<Type<T>> {
  static_assert((Type<T>{}, false));
};

template <typename T>
auto type = Type<T>{};

template <typename A, typename B>
constexpr bool operator==(Type<A>, Type<B>) {
  return std::is_same_v<A, B>;
}

template <typename A, typename B>
constexpr bool operator!=(Type<A>, Type<B>) {
  return !(type<A> == type<B>);
}

template <typename A>
constexpr auto removePointer(Type<A>) -> Type<std::remove_pointer_t<A>> {
  return {};
}

template <typename A>
constexpr auto sizeOf(Type<A>) -> size_t {
  return sizeof(A);
}

template <typename T>
auto unwrap(Type<T>) -> T;

// 2. Variadic templates
// Doing exactly the same as in 1. but for type packs. Please compare the functions and
// operators. Variadic templates (...)
// ...T (pack into pack): eg. T = "(T1, T2, T3)" (T is a single entity - a type pack)
// T... (unpack from pack): T = (T1, T2, T3) turns into "T1, T2, T3" (braces removed, now
// you have 3 type entities.)

template <typename... T>
struct TypePack {};

template <typename... T>
auto typePack = TypePack<T...>{};

template <typename... A, typename B>
constexpr auto append(TypePack<A...>, Type<B>) -> TypePack<A..., B> {
  return {};
}

template <typename... A, typename... B>
constexpr bool operator==(TypePack<A...>, TypePack<B...>) {
  if constexpr (sizeof...(A) != sizeof...(B)) {
    return false;
  } else {
    return ((type<A> == type<B>)&&...);
  }
}

// map function. Turn a type pack of one type to a type pack of possibly another type.
// eg. (A, A, A) -> (B, B, B)
// NOT: (A, B, C) -> (A', B', C')
// decltype(unwrap(f(type<T>)))
// eg. T = int and f turns an integer into a double (Type(int) -> Type(double)):
// f(Type<int>) ->apply-> Type<double> ->unwrap-> double ->decltype-> double as a type
template <typename... T, typename F>
constexpr auto transform(TypePack<T...>, F&& f)
    -> TypePack<decltype(unwrap(f(type<T>)))...> {
  return {};
}

// 3. Values as types

// Old style (before C++17)
template <class T, T V>
struct integral_constant {
  constexpr static T value = V;
  using value_type = T;
  // Conversion operators
  // convert to value_type (from integral_constant<T, V)
  constexpr operator value_type() const noexcept { return value; }
  // call operator (for integral_constant<T, V>)
  constexpr value_type operator() () const noexcept { return value; }
};

// C++17 style
template<auto V>
struct Value {
  static constexpr auto value = V;
  using value_type = decltype(V);
  // Only one conversion operator
  // auto operator!!! Pretty!!!
  constexpr operator auto() const noexcept { return value; }
};

// add two numbers
template<typename A, typename B>
using Add = Value<A::value + B::value>;

// float, double, string are not integral types and are not allowed as template parameters
// using Pi = Value<3.14159>;
// using Hello = Value<"Hello">;

// 1. Function pointers are integral values!
// 2. lambda who don't capture anything can be converted to function pointers

// Specialisation for a function pointer with no arguments.
// We don't care about the return value ("auto").
template<auto (*F)()>
struct Value<F> {
  static constexpr auto value = F();
  using value_type = decltype(value);
  constexpr operator auto() const noexcept { return value; }
};

// Now we can write Pi as a lambda.
// But compiler doesn't not know we want to convert our lambda to a function pointer!
// using Pi = Value<[]() {return 3.14159;}>;
// We can use a trick: plus sign drives!!! This forces exactly this conversion!
using Pi = Value<+[]() {return 3.14159;}>;

// empty braces actually not needed
using Hello = Value<+[] { return "Hello"; }>;

auto testHello() -> const char* {
  // return an instance of our Hello
  return Hello{};
}

template <typename... A>
using Sum = Value<+[] { return (A::value + ...); }>;

// Return type (double) needed. Otherwise conversion would not be triggered.
auto testSum() -> double{
  using FortyTwo = Value<42>;
  using Pi = Value<+[]() { return 3.14159; }>;
  using V2 = Value<+[] {return -23;}>;
  return Sum<FortyTwo, V2, Pi>{};
};

// 4. Application of Values as Types

// Packed Options
// One integer stands for error, everything else success.
// eg. in C legacy applications where return value of -1 is error, anything else is a return code.
template <typename T>
struct Optional { /*...*/
};

template<auto V>
struct Optional<Value<V>>{
  constexpr static auto invalidValue = Value<V>::value;
  using value_type = decltype(invalidValue);

  constexpr operator bool() const{
    return value != invalidValue;
  }

  constexpr auto reset() {
    value = invalidValue;
  }

  constexpr auto get() const { return value; }

  constexpr auto set(value_type newValue){
    value = newValue;
  }

private:
  value_type value{invalidValue};
};

auto testOptional() {
  using DefaultName = Value<+[]{return "Max Mustermann";}>;
  using OptName = Optional<DefaultName>;

  constexpr auto defaulted = OptName{};
  static_assert(!defaulted);
  static_assert(defaulted.get() == DefaultName::value);
}

// Tests

auto testTypes() {
  Type<int> someInt;
  static_assert(someInt == type<int32_t>);
}

auto testPointer() {
  auto voidPointer = type<void*>;
  static_assert(type<void> == removePointer(voidPointer));
}

auto testEquality() {
  Type<int> anInt;
  Type<int> anotherInt;
  Type<double> aDouble;
  static_assert(anInt == anotherInt);
  static_assert(anInt != aDouble);
}

auto testTypeSize() {
  Type<int> anInt;

  static_assert(sizeOf(anInt) == 4);
  static_assert(sizeOf(type<int>) == 4);
  std::cout << "Size of int: " << sizeOf(type<int>) << "\n";
}

auto testUnwrap() {
  auto intType = type<int>;
  auto value = decltype(unwrap(intType)){23};
  return value;
}

auto testTypePack() {
  auto intCharFloat = typePack<int, char, float>;
  auto intCharFloatDouble = append(intCharFloat, type<double>);
  static_assert(intCharFloatDouble == typePack<int, char, float, double>);
}

auto testTransform() {
  auto input = typePack<int, void*>;
  auto output = transform(input, [](auto t){
      return removePointer(t);
    });
  static_assert(output == typePack<int, void>);
}

auto testAdd() {
  // old style
  using OneOld = integral_constant<int, 1>;

  // C++17 style
  using One = Value<1>;
  using FortyTwo = Value<42>;

  static_assert(Add<One, FortyTwo>::value == 43);

}

/*
auto testRecursiveTypes() {
  using typeOftypeInt = Type<Type<int>>;
  typeOftypeInt nestedInt;
  std::cout << "Size if nested Int: " << sizeOf(nestedInt) << ". (Wrong! Would need to
recurse.)\n";
}
*/

int main() {
  std::cout << "Just testing.\n";

  testTypes();
  testPointer();
  testEquality();
  testTypeSize();
  testUnwrap();
  testTypePack();
  testTransform();

  return 0;
}
