#pragma once
#ifndef ANY_H_
#define ANY_H_

#include <exception>
#include <memory>
#include <typeinfo>
#include <type_traits>


namespace details {

class bad_any_cast final : public std::runtime_error {
public:
  bad_any_cast() : std::runtime_error("bad_any_cast")
  { }

  bad_any_cast(const char* str) : std::runtime_error(str)
  { }
};


struct IAnyBase {
  virtual std::unique_ptr<IAnyBase> clone() = 0;
  virtual const std::type_info& type()      = 0;
  virtual ~IAnyBase()                       = default;
};

template <typename T>
struct AnyImpl final : public IAnyBase {
  T data_;

  template <typename... Args>
  AnyImpl(Args&&... args) : data_{std::forward<Args>(args)...}
  { }

  std::unique_ptr<IAnyBase> clone() override {
    return std::make_unique<AnyImpl<T>>(data_);
  }

  const std::type_info& type() override {
    return typeid(data_);
  }
};

} // namespace details


class Any final {
  template <typename U>
  friend U any_cast(Any& object);

private:
  std::unique_ptr<details::IAnyBase> storage_{nullptr};

public:
  constexpr Any() noexcept;
  Any(const Any& rhs);
  Any(Any&& rhs) noexcept;
  template <typename T,
            typename TaD = std::decay_t<T>,
            typename = std::enable_if_t<!std::is_same_v<TaD, Any>>>
  Any(T&& data);

  ~Any();

  Any& operator=(const Any& rhs);
  Any& operator=(Any&& rhs) noexcept;
  template <typename T,
            typename TaD = std::decay_t<T>,
            typename = std::enable_if_t<!std::is_same_v<TaD, Any>>>
  Any& operator=(T&& data);

  template <typename T, typename... Args>
  std::decay_t<T>& emplace(Args&&... args);
  void reset() noexcept;
  void swap(Any& rhs) noexcept;
  bool has_value() const noexcept;
  const std::type_info& type() const;
};

void swap(Any& lhs, Any& rhs) noexcept;

template <typename T>
T any_cast(Any& object);
// Any declaration //---------------------------------------------------//

// Any definition //----------------------------------------------------//
constexpr Any::Any() noexcept = default;

Any::Any(const Any& rhs) : storage_{rhs.storage_->clone()}
{ }

Any::Any(Any&& rhs) noexcept : storage_{std::move(rhs.storage_)}
{ }

template <typename T, typename TaD, typename>
Any::Any(T&& data)
    : storage_{std::make_unique<details::AnyImpl<T>>(std::forward<T>(data))}
{ }

Any::~Any() = default;

Any& Any::operator=(const Any& rhs) {
  Any(rhs).swap(*this);
  return *this;
}
Any& Any::operator=(Any&& rhs) noexcept {
  Any(std::move(rhs)).swap(*this);
  return *this;
}

template <typename T, typename TaD, typename>
Any& Any::operator=(T&& data) {
  Any(std::forward<T>(data)).swap(*this);
  return *this;
}

template <typename T, typename... Args>
std::decay_t<T>& Any::emplace(Args&&... args) {
  storage_ = std::make_unique<details::AnyImpl<T>>(std::forward<Args>(args)...);
  return reinterpret_cast<details::AnyImpl<T>*>(storage_.get())->data_;
}

void Any::reset() noexcept {
  storage_ = nullptr;
}

void Any::swap(Any& rhs) noexcept {
  std::swap(storage_, rhs.storage_);
}

bool Any::has_value() const noexcept {
  return storage_ != nullptr;
}

const std::type_info& Any::type() const {
  return storage_->type();
}

void swap(Any& lhs, Any& rhs) noexcept {
  lhs.swap(rhs);
}

template <typename T>
T any_cast(Any& object) {
  if (object.type() == typeid(T)) {
    return reinterpret_cast<details::AnyImpl<T>*>(object.storage_.get())->data_;
  } else {
    throw details::bad_any_cast("fanction any_cast: wrong type");
  }
}
// Any definition //----------------------------------------------------//

#endif // ANY_H_
