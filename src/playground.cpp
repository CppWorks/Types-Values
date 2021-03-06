#include <cassert>
#include <cinttypes>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>
#include <tuple>

// 1. Types as values

// just a wrapper for types is a new type called Type<T>. T is the wrappee.
template <typename T>
struct Type {};

// disallow recursive types
template <typename T>
struct Type<Type<T>> {
  static_assert((Type<T>{}, false));
};

// does not compile anymore
/*
  auto testRecursiveTypes() {
    using typeOftypeInt = Type<Type<int>>;
    typeOftypeInt nestedInt;
    std::cout << "Size if nested Int: " << sizeOf(nestedInt)
              << ". (Wrong! Would need to recurse.)\n";
  }
*/

// helper function to create instances of Type<T>
template <typename T>
auto type = Type<T>{};

// compare types
template <typename A, typename B>
constexpr bool operator==(Type<A>, Type<B>) {
  return std::is_same_v<A, B>;
}

template <typename A, typename B>
constexpr bool operator!=(Type<A>, Type<B>) {
  return !(type<A> == type<B>);
}

auto testTypes() {
  Type<int> anInt;
  Type<int> anotherInt;
  Type<double> aDouble;
  static_assert(anInt == type<int32_t>);
  static_assert(anInt == anotherInt);
  static_assert(anInt != aDouble);
}

// pointer to types
template <typename A>
constexpr auto removePointer(Type<A>) -> Type<std::remove_pointer_t<A>> {
  return {};
}

auto testPointer() {
  auto voidPointer = type<void*>;
  static_assert(type<void> == removePointer(voidPointer));
}

// how big is a type
template <typename A>
constexpr auto sizeOf(Type<A>) -> size_t {
  return sizeof(A);
}

auto testTypeSize() {
  Type<int> anInt;
  static_assert(sizeOf(anInt) == 4);
  static_assert(sizeOf(type<int>) == 4);
  std::cout << "Size of int: " << sizeOf(type<int>) << "\n";
}

// declaration without implementation worked too. "return {}" seems to be like a default return.
// constexpr not needed either.
template <typename T>
auto unwrap(Type<T>) -> T;

// get the original type out of Type<T>. (Not needed actually. Declaration is sufficient.)
template <typename T>
auto unwrap(Type<T>) -> T {
  return {};
};

auto testUnwrap() {
  auto intType = type<int>;
  auto value = decltype(unwrap(intType)){23};
  return value;
}

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

auto testTypePack() {
  auto intCharFloat = typePack<int, char, float>;
  auto intCharFloatDouble = append(intCharFloat, type<double>);
  static_assert(intCharFloatDouble == typePack<int, char, float, double>);
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

auto testTransform() {
  auto input = typePack<int, void*>;
  auto output = transform(input, [](auto t){
      return removePointer(t);
    });
  static_assert(output == typePack<int, void>);
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

// C++17 style. Everything auto.
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

auto testAdd() {
  // old style
  using OneOld = integral_constant<int, 1>;

  // C++17 style
  using One = Value<1>;
  using FortyTwo = Value<42>;

  static_assert(Add<One, FortyTwo>::value == 43);
}

// float, double, string are not integral types and are not allowed as template parameters
// using Pi = Value<3.14159>;
// using Hello = Value<"Hello">;

// Specialisation for a function pointer with no arguments.
// We don't care about the return value ("auto").
template<auto (*F)()>
struct Value<F> {
  static constexpr auto value = F();
  using value_type = decltype(value);
  constexpr operator auto() const noexcept { return value; }
};

// 1. Function pointers are integral values!
// 2. lambda who don't capture anything can be converted to function pointers
// https://stackoverflow.com/questions/43843550/template-argument-deduction-from-lambda
// We can use a trick: unary +

// Now we can write Pi as a lambda.
// using Pi = Value<[]() {return 3.14159;}>;
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

// a. Packed Options
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

// b. Callable (scripting interface for C++ functions)

using Memory = std::vector<uint8_t>;

template<class R, class... Args>
constexpr auto argumentTuple(R (*)(Args...)) -> std::tuple<Args...> {
  return {};
}

template<auto F>
struct Callable {
  using Tuple = decltype(argumentTuple(F));

  constexpr static auto size = sizeof(Tuple);

  constexpr static auto asArguments(Memory& m) -> Tuple& {
    assert(m.size() == size);
    return reinterpret_cast<Tuple&>(m[0]);
  }

  // call operator
  // the arguments are in Memory
  constexpr auto operator()(Memory& m) const {
    //std::apply takes a function and a tuple of arguments
    return std::apply(F, asArguments(m));
  }
};

auto testCallable() {
  constexpr auto inc = Callable<+[](int& i) { return i += 1; }>{};
  auto m = Memory{};
  // initialize memory
  m.resize(inc.size);
  int i = 42;
  // casting not so nice here
  // we need a writable pointer to i
  // alternative: *reinterpret_cast<int**>(m.data()) = &i;
  reinterpret_cast<int*&>(m[0]) = &i;

  return inc(m);
}

int main() {}
