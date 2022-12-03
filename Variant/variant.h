#pragma once
#ifndef VARIANT_H_
#define VARIANT_H_

#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>


namespace details {

// the some type for initialize empty variant
struct EmpyType final { };

// the some value for initialize empty variant
constexpr std::size_t variant_npos = static_cast<std::size_t>(-1);

class bad_variant_access final : public std::runtime_error {
public:
  bad_variant_access() : std::runtime_error("bad_variant_access")
  { }

  bad_variant_access(const char* str) : std::runtime_error(str)
  { }
};

// index_by_type //-----------------------------------------------------//
template <std::size_t Index, typename T, typename Head, typename... Tail>
struct index_by_type
    : public std::integral_constant<std::size_t, std::is_same_v<T, Head>
             ? Index : index_by_type<Index+1, T, Tail...>::value>
{ };

template <std::size_t Index, typename T, typename Last>
struct index_by_type<Index, T, Last>
    : public std::integral_constant<std::size_t, std::is_same_v<T, Last>
             ? Index : variant_npos>
{ };

template <typename T, typename... Types>
static constexpr std::size_t index_by_type_v =
  index_by_type<0, T, Types...>::value;
//----------------------------------------------------------------------//

// type_by_index //-----------------------------------------------------//
template <std::size_t Index, typename Head, typename... Tail>
struct type_by_index {
    using type = typename type_by_index<Index - 1, Tail...>::type;
};

template <typename Head, typename... Tail>
struct type_by_index<0, Head, Tail...> {
    using type = Head;
};

template <size_t Index, typename... Types>
using type_by_index_t = typename type_by_index<Index, Types...>::type;
//----------------------------------------------------------------------//

// check_is_contain //--------------------------------------------------//
template <typename T, typename... Types>
struct check_is_contain
    : public std::bool_constant<(std::is_same_v<T, Types> || ...)>
{ };

template <typename T>
struct check_is_contain<T> : public std::bool_constant<false>
{ };

template <typename T, typename... Types>
static constexpr bool check_is_contain_v =
  check_is_contain<T, Types...>::value;
//----------------------------------------------------------------------//

// VariadicUnion //-----------------------------------------------------//
template <typename... Types>
union VariadicUnion {
  EmpyType empty{};

  VariadicUnion() { }
  ~VariadicUnion() { }

  template <std::size_t Index, typename... Args>
  void put(Args&&...) { }
};

template <typename Head, typename... Tail>
union VariadicUnion<Head, Tail...> {
  Head head_;
  VariadicUnion<Tail...> tail_;

  VariadicUnion() { }
  VariadicUnion(const VariadicUnion&) { }
  VariadicUnion(VariadicUnion&&) { }
  ~VariadicUnion() { }
  VariadicUnion& operator=(const VariadicUnion&) { }
  VariadicUnion& operator=(VariadicUnion&&) { }

  template <std::size_t Index, typename... Args>
  void put(Args&&... args) {
    if constexpr (Index == 0) {
      static_assert(std::is_constructible_v<Head, Args...>,
                    "can't construct the type from passed arguments");
      ::new(std::launder(&head_)) Head(std::forward<Args>(args)...);
    } else {
      tail_. template put<Index-1>(std::forward<Args>(args)...);
    }
  }

  template <size_t Index>
  constexpr type_by_index_t<Index, Head, Tail...>& get_lvalue_ref() {
    if constexpr (Index == 0) {
      return head_;
    } else {
      return tail_.template get_lvalue_ref<Index-1>();
    }
  }

  template <size_t Index>
  constexpr const type_by_index_t<Index, Head, Tail...>&
  get_const_lvalue_ref() const {
    if constexpr (Index == 0) {
      return head_;
    } else {
      return tail_.template get_const_lvalue_ref<Index-1>();
    }
  }

  template <size_t Index>
  constexpr type_by_index_t<Index, Head, Tail...>&& get_rvalue_ref() {
    if constexpr (Index == 0) {
      return std::move(head_);
    } else {
      return std::move(tail_.template get_rvalue_ref<Index-1>());
    }
  }

  template <size_t Index>
  constexpr const type_by_index_t<Index, Head, Tail...>&&
  get_const_rvalue_ref() const {
    if constexpr (Index == 0) {
      return std::move(head_);
    } else {
      return std::move(tail_.template get_const_rvalue_ref<Index-1>());
    }
  }

  template <typename T>
  void destroy() {
    if constexpr (std::is_same_v<T, Head>) {
      head_.~Head();
    }
  }
}; // VariadicUnion //--------------------------------------------------//

} // namespace details //-----------------------------------------------//

// VariantStorage //----------------------------------------------------//

// struct for default initializing Variant fields before they are used by
// VariantAlternativ
template <typename... Types>
struct VariantStorage {
  details::VariadicUnion<Types...> storage_{};
  std::size_t current_index_{};
}; // VariantStorage //-------------------------------------------------//

template <typename... Types>
class Variant;

// VariantAlternative //------------------------------------------------//
template <typename T, typename... Types>
struct VariantAlternative {
  using Derived = Variant<Types...>;
  static constexpr std::size_t kIndex = details::index_by_type_v<T, Types...>;

  Derived* cast_to_derived() {
    return static_cast<Derived*>(this);
  }

  VariantAlternative() { }

  VariantAlternative(const T& value) {
    auto ptr = cast_to_derived();
    ptr-> template safe_put<kIndex>(value);
  }

  VariantAlternative(T&& value) {
    auto ptr = cast_to_derived();
    ptr-> template safe_put<kIndex>(value);
  }

  ~VariantAlternative() { }

  auto& operator=(const T& value) {
    auto ptr = cast_to_derived();
    if (ptr->current_index_ == kIndex) {
      ptr-> template safe_assign<kIndex>(value);
    } else {
      ptr->destroy_all();
      ptr->template safe_put<kIndex>(value);
    }

    return *ptr;
  }

  auto& operator=(T&& value) {
    auto ptr = cast_to_derived();
    if (ptr->current_index_ == kIndex) {
      ptr-> template safe_assign<kIndex>(std::move(value));
    } else {
      ptr->destroy_all();
      ptr->template safe_put<kIndex>(std::move(value));
    }

    return *ptr;
  }

  VariantAlternative(const Derived& rhs) {
    if (rhs.current_index_ != kIndex)
      return;
 
    auto ptr = cast_to_derived();
    ptr->template safe_put<kIndex>(get<kIndex>(rhs));
  }
 
  VariantAlternative(Derived&& rhs) {
    if (rhs.current_index_ != kIndex)
      return;
 
    auto ptr = cast_to_derived();
    ptr->template safe_put<kIndex>(std::move(get<kIndex>(std::move(rhs))));
  }

  void assign(const Derived& rhs) {
    if (kIndex != rhs.current_index_)
      return;

    auto ptr = cast_to_derived();
    if (ptr->current_index_ == kIndex) {
      ptr-> template safe_assign<kIndex>(get<kIndex>(rhs));
    } else {
      ptr->destroy_all();
      ptr->template safe_put<kIndex>(get<kIndex>(rhs));
    }
  }

  void assign(Derived&& rhs) {
    if (kIndex != rhs.current_index_)
      return;

    auto ptr = cast_to_derived();
    if (ptr->current_index_ == kIndex) {
      ptr-> template safe_assign<kIndex>(std::move(get<kIndex>(rhs)));
    } else {
      ptr->destroy_all();
      ptr->template safe_put<kIndex>(std::move(get<kIndex>(std::move(rhs))));
    }
  }

  void destroy() {
    auto ptr = cast_to_derived();
    if (ptr->current_index_ == kIndex) {
      ptr->storage_.template destroy<T>();
    }
  }
}; // VariantAlternative //---------------------------------------------//


template <typename... Types>
class Variant final : private VariantStorage<Types...>,
                      private VariantAlternative<Types, Types...>... {
private:
  template <typename T, typename... Ts>
  friend struct VariantAlternative;

  template <typename T, typename... Ts>
  friend constexpr bool
  holds_alternative(const Variant<Ts...>& variant) noexcept;

  template <size_t Index, typename... Ts>
  friend constexpr details::type_by_index_t<Index, Ts...>&
  get(Variant<Ts...>& variant);

  template <size_t Index, typename... Ts>
  friend constexpr const details::type_by_index_t<Index, Ts...>&
  get(const Variant<Ts...>& variant);

  template <size_t Index, typename... Ts>
  friend constexpr details::type_by_index_t<Index, Ts...>&&
  get(Variant<Ts...>&& variant);

  template <size_t Index, typename... Ts>
  friend constexpr const details::type_by_index_t<Index, Ts...>&&
  get(const Variant<Ts...>&& variant);

public:
  using VariantStorage<Types...>::storage_;
  using VariantStorage<Types...>::current_index_;
  using VariantAlternative<Types, Types...>::VariantAlternative...;
  using VariantAlternative<Types, Types...>::operator=...;

private:
  //details::VariadicUnion<Types...> storage_;
  //std::size_t current_index_;

private:
  constexpr void destroy_all();

  template <std::size_t Index, typename... Args>
  constexpr void safe_put(Args&&... args);

  template <std::size_t Index, typename T>
  constexpr void safe_assign(const T& value);

public:
  constexpr Variant();
  constexpr Variant(const Variant& rhs);
  constexpr Variant(Variant&& rhs);

  ~Variant();

  constexpr Variant& operator=(const Variant& rhs);
  constexpr Variant& operator=(Variant&& rhs);

  constexpr std::size_t index() const noexcept;
  constexpr bool valueless_by_exception() const noexcept;

  template <std::size_t Index, typename... Args>
  constexpr details::type_by_index_t<Index, Types...>&
  emplace(Args&&... args);

  template <typename T, typename... Args>
  constexpr T& emplace(Args&&... args);
};

template <typename T, typename... Types>
constexpr bool holds_alternative(const Variant<Types...>& variant) noexcept;

template <size_t Index, typename... Ts>
constexpr details::type_by_index_t<Index, Ts...>&
get(Variant<Ts...>& variant);

template <size_t Index, typename... Ts>
constexpr const details::type_by_index_t<Index, Ts...>&
get(const Variant<Ts...>& variant);

template <size_t Index, typename... Ts>
constexpr details::type_by_index_t<Index, Ts...>&&
get(Variant<Ts...>&& variant);

template <size_t Index, typename... Ts>
constexpr const details::type_by_index_t<Index, Ts...>&&
get(const Variant<Ts...>&& variant);

template <typename T, typename... Types>
constexpr T& get(Variant<Types...>& variant);

template <typename T, typename... Types>
constexpr const T& get(const Variant<Types...>& variant);

template <typename T, typename... Types>
constexpr T&& get(Variant<Types...>&& variant);

template <typename T, typename... Types>
constexpr const T&& get(const Variant<Types...>&& variant);
// Variant declaration //-----------------------------------------------//

// Variant definition //------------------------------------------------//
template <typename... Types>
constexpr void Variant<Types...>::destroy_all() {
  (VariantAlternative<Types, Types...>::destroy(), ...);
}

template <typename... Types>
template <std::size_t Index, typename... Args>
constexpr void Variant<Types...>::safe_put(Args&&... args) {
  try {
    storage_.template put<Index>(std::forward<Args>(args)...);
    current_index_ = Index;
  } catch (...) {
    storage_.template put<details::variant_npos>();
    current_index_ = details::variant_npos;
    throw;
  }
}

template <typename... Types>
template <std::size_t Index, typename T>
constexpr void Variant<Types...>::safe_assign(const T& value) {
  try {
    get<Index>(*this) = value;
  } catch (...) {
    storage_.template put<details::variant_npos>();
    current_index_ = details::variant_npos;
    throw;
  }
}

template <typename... Types>
constexpr Variant<Types...>::Variant() {
  storage_.template put<0>();
  current_index_ = 0;
}

template <typename... Types>
constexpr Variant<Types...>::Variant(const Variant& rhs)
    : VariantAlternative<Types, Types...>(rhs)...
{ }

template <typename... Types>
constexpr Variant<Types...>::Variant(Variant&& rhs)
    : VariantAlternative<Types, Types...>(std::move(rhs))...
{ }

template <typename... Types>
constexpr Variant<Types...>&
Variant<Types...>::operator=(const Variant& rhs) {
  (VariantAlternative<Types, Types...>::assign(rhs),...);
  return *this;
}

template <typename... Types>
constexpr Variant<Types...>&
Variant<Types...>::operator=(Variant&& rhs) {
  (VariantAlternative<Types, Types...>::assign(std::move(rhs)),...);
  return *this;
}

template <typename... Types>
Variant<Types...>::~Variant() {
  destroy_all();
}

template <typename... Types>
constexpr std::size_t Variant<Types...>::index() const noexcept {
  return valueless_by_exception() ? details::variant_npos : current_index_;
}

template <typename... Types>
constexpr bool Variant<Types...>::valueless_by_exception() const noexcept {
  return current_index_ == details::variant_npos;
}

template <typename... Types>
template <std::size_t Index, typename... Args>
constexpr details::type_by_index_t<Index, Types...>&
Variant<Types...>::emplace(Args&&... args) {
  static_assert(Index < sizeof...(Types),
              "the index is greater than the pack size of types of variant");
  destroy_all();
  safe_put<Index>(std::forward<Args>(args)...);
  return get<Index>(*this);
}

template <typename... Types>
template <typename T, typename... Args>
constexpr T& Variant<Types...>::emplace(Args&&... args) {
  static_assert(details::check_is_contain_v<T, Types...>,
               "function holds_alternative: varinat don't contain this type");
  constexpr std::size_t index = details::index_by_type_v<T, Types...>;
  return emplace<index>(std::forward<Args>(args)...);
}

template <typename T, typename... Types>
constexpr bool holds_alternative(const Variant<Types...>& variant) noexcept {
  static_assert(details::check_is_contain_v<T, Types...>,
               "function holds_alternative: varinat don't contain this type");
  constexpr std::size_t index = details::index_by_type_v<T, Types...>;
  return index == variant.current_index_;
}

template <size_t Index, typename... Types>
constexpr details::type_by_index_t<Index, Types...>&
get(Variant<Types...>& variant) {
  static_assert(Index < sizeof...(Types),
              "the index is greater than the pack size of types of variant");
  if (variant.current_index_ != Index)
    throw details::bad_variant_access("function get: wrong index for variant");
  return variant.storage_.template get_lvalue_ref<Index>();
}

template <size_t Index, typename... Types>
constexpr const details::type_by_index_t<Index, Types...>&
get(const Variant<Types...>& variant) {
  static_assert(Index < sizeof...(Types),
              "the index is greater than the pack size of types of variant");
  if (variant.current_index_ != Index)
    throw details::bad_variant_access("function get: wrong index for variant");
  return variant.storage_.template get_const_lvalue_ref<Index>();
}

template <size_t Index, typename... Types>
constexpr details::type_by_index_t<Index, Types...>&&
get(Variant<Types...>&& variant) {
  static_assert(Index < sizeof...(Types),
              "the index is greater than the pack size of types of variant");
  if (variant.current_index_ != Index)
    throw details::bad_variant_access("function get: wrong index for variant");
  return variant.storage_.template get_rvalue_ref<Index>();
}

template <size_t Index, typename... Types>
constexpr const details::type_by_index_t<Index, Types...>&&
get(const Variant<Types...>&& variant) {
  static_assert(Index < sizeof...(Types),
              "the index is greater than the pack size of types of variant");
  if (variant.current_index_ != Index)
    throw details::bad_variant_access("function get: wrong index for variant");
  return variant.storage_.template get_const_rvalue_ref<Index>();
}

template <typename T, typename... Types>
constexpr T& get(Variant<Types...>& variant) {
  if (!holds_alternative<T>(variant))
    throw details::bad_variant_access("the value type is incorrect");

  constexpr std::size_t index = details::index_by_type_v<T, Types...>;
  return get<index>(variant);
}

template <typename T, typename... Types>
constexpr const T& get(const Variant<Types...>& variant) {
  if (!holds_alternative<T>(variant))
    throw details::bad_variant_access("the value type is incorrect");

  constexpr std::size_t index = details::index_by_type_v<T, Types...>;
  return get<index>(variant);
}

template <typename T, typename... Types>
constexpr T&& get(Variant<Types...>&& variant) {
  if (!holds_alternative<T>(variant))
    throw details::bad_variant_access("varinat don't contain this type");

  constexpr std::size_t index = details::index_by_type_v<T, Types...>;
  return get<index>(std::move(variant));
}

template <typename T, typename... Types>
constexpr const T&& get(const Variant<Types...>&& variant) {
  if (!holds_alternative<T>(variant))
    throw details::bad_variant_access("varinat don't contain this type");

  constexpr std::size_t index = details::index_by_type_v<T, Types...>;
  return get<index>(std::move(variant));
}

// Variant definition //------------------------------------------------//

#endif // VARIANT_H_
