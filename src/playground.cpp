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

// 3. Values as types (37:35)

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
