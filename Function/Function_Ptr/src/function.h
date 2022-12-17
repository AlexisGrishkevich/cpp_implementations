// this function class is implemented on pointers to functions
#pragma once
#ifndef FUNCTION_H_
#define FUNCTION_H_

#include <cstddef>
#include <functional>
#include <memory>


template <typename>
class Function;

template <typename RetType, typename... Args>
class Function<RetType(Args...)> final {
private:
  using invoke_ptr_t = RetType (*)(void*, Args&&...);
  using construct_ptr_t = void (*)(void*, void*);
  using destroy_ptr_t = void (*)(void*);

private:
  invoke_ptr_t invoke_ptr{nullptr};
  construct_ptr_t construct_ptr{nullptr};
  destroy_ptr_t destroy_ptr{nullptr};

  std::size_t storage_size_{0};
  std::unique_ptr<char[]> storage_{nullptr};

private:
  template <typename Functor>
  static RetType invoke(Functor* functor, Args&&... args);

  template <typename Functor>
  static void construct(Functor* destination, Functor* source);

  template <typename Functor>
  static void destroy(Functor* functor);

public:
  Function();
  template <typename Functor>
  Function(Functor functor);
  Function(const Function& rhs);

  ~Function();

  Function& operator=(const Function& rhs);
  RetType operator()(Args&&... args) const;
  operator bool();
};
// Function declaration //----------------------------------------------//

// Function definition //-----------------------------------------------//
template <typename RetType, typename... Args>
template <typename Functor>
RetType Function<RetType(Args...)>::invoke(Functor* functor, Args&&... args) {
  return std::invoke(*functor, std::forward<Args>(args)...);
}

template <typename RetType, typename... Args>
template <typename Functor>
void Function<RetType(Args...)>::construct(Functor* destination, Functor* source) {
  new (destination) Functor(*source);
}

template <typename RetType, typename... Args>
template <typename Functor>
void Function<RetType(Args...)>::destroy(Functor* functor) {
  functor->~Functor();
}


template <typename RetType, typename... Args>
Function<RetType(Args...)>::Function() = default;

template <typename RetType, typename... Args>
template <typename Functor>
Function<RetType(Args...)>::Function(Functor functor)
    : invoke_ptr{reinterpret_cast<invoke_ptr_t>(invoke<Functor>)},
      construct_ptr{reinterpret_cast<construct_ptr_t>(construct<Functor>)},
      destroy_ptr{reinterpret_cast<destroy_ptr_t>(destroy<Functor>)},
      storage_size_{sizeof(Functor)},
      storage_{new char[storage_size_]} {
  construct_ptr(storage_.get(), std::addressof(functor));
}

template <typename RetType, typename... Args>
Function<RetType(Args...)>::Function(const Function& rhs)
    : invoke_ptr{rhs.invoke_ptr}, construct_ptr{rhs.construct_ptr},
      destroy_ptr{rhs.destroy_ptr}, storage_size_{rhs.storage_size_} {
  if (invoke_ptr) {
    storage_.reset(new char[storage_size_]);
    construct_ptr(storage_.get(), rhs.storage_.get());
  }
}

template <typename RetType, typename... Args>
Function<RetType(Args...)>::~Function() {
  if (storage_)
    destroy_ptr(storage_.get());
}

template <typename RetType, typename... Args>
Function<RetType(Args...)>&
Function<RetType(Args...)>::operator=(const Function& rhs) {
  if (storage_) {
    destroy_ptr(storage_.get());
    storage_.reset();
  }

  if (rhs.storage_) {
    invoke_ptr = rhs.invoke_ptr;
    construct_ptr = rhs.construct_ptr;
    destroy_ptr = rhs.destroy_ptr;

    storage_size_ = rhs.storage_size_;
    storage_.reset(new char[storage_size_]);

    construct_ptr(storage_.get(), rhs.storage_.get());
  }

  return *this;
}


template <typename RetType, typename... Args>
RetType Function<RetType(Args...)>::operator()(Args&&... args) const {
  return invoke_ptr(storage_.get(), std::forward<Args>(args)...);
}

template <typename RetType, typename... Args>
Function<RetType(Args...)>::operator bool() {
  return storage_ != nullptr;
}
// Function definition //-----------------------------------------------//

#endif // FUNCTION_H_
