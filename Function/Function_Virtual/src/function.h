// this function class is implemented on virtual function
#pragma once
#ifndef FUNCTION_H_
#define FUNCTION_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>


namespace details {

template <typename RetType, typename... Args>
struct IFunctionBase {
  virtual ~IFunctionBase()                              = default;
  virtual RetType operator()(Args&&... args)            = 0;
  virtual void destroy()                                = 0;
  virtual void construct(IFunctionBase* my_destination) = 0;
  virtual IFunctionBase* clone()const                   = 0;
};

template <typename Functor, typename RetType, typename... Args>
struct FunctionImpl final : public IFunctionBase<RetType, Args...> {
  Functor f_;

  FunctionImpl(Functor f) : f_{f} { }

  RetType operator()(Args&&... args) override {
    return std::invoke(f_, std::forward<Args>(args)...);
  }

  void construct(IFunctionBase<RetType, Args...>* my_destination) override {
    new (my_destination) FunctionImpl<Functor, RetType, Args...>(f_);
  }

  void destroy() override {
    f_.~Functor();
  }

  IFunctionBase<RetType, Args...>* clone() const override {
    return new FunctionImpl<Functor, RetType, Args...>(f_);
  }
};

} // namespace details


template <typename>
class Function;

template <typename RetType, typename... Args>
class Function<RetType(Args...)> final {
public:
  using FunctionBase = details::IFunctionBase<RetType, Args...>;

private:
  mutable std::aligned_storage_t<24> stack_{};
  FunctionBase* storage_ptr_{nullptr};

public:
  Function();
  template <typename Functor>
  Function(Functor f);
  Function(const Function& rhs);

  ~Function();

  Function& operator=(const Function& rhs);
  RetType operator()(Args&&... args) const;
  operator bool();
};
// Function declaration //----------------------------------------------//

// Function definition //-----------------------------------------------//
template <typename RetType, typename... Args>
Function<RetType(Args...)>::Function() = default;

template <typename RetType, typename... Args>
template <typename Functor>
Function<RetType(Args...)>::Function(Functor f) {
  using FunctionImpl = details::FunctionImpl<Functor, RetType, Args...>;
  if (sizeof(f) <= sizeof(stack_)) {
    storage_ptr_ =
      reinterpret_cast<decltype(storage_ptr_)>(std::addressof(stack_));
    new (storage_ptr_) FunctionImpl(f);
  } else {
    storage_ptr_ = new FunctionImpl(f);
  }
}

template <typename RetType, typename... Args>
Function<RetType(Args...)>::Function(const Function& rhs) {
  if (rhs.storage_ptr_ != nullptr) {
    if (rhs.storage_ptr_ == reinterpret_cast<decltype(rhs.storage_ptr_)>(
                            std::addressof(rhs.stack_))) {
      storage_ptr_ = reinterpret_cast<decltype(storage_ptr_)>(
                     std::addressof(stack_));
      rhs.storage_ptr_->construct(storage_ptr_);
    } else {
      storage_ptr_ = rhs.storage_ptr_->clone();
    }
  }
}

template <typename RetType, typename... Args>
Function<RetType(Args...)>::~Function() {
  if (storage_ptr_ == reinterpret_cast<decltype(storage_ptr_)>(
                      std::addressof(stack_))) {
    storage_ptr_->destroy();
  } else {
    delete storage_ptr_;
  }
}

template <typename RetType, typename... Args>
Function<RetType(Args...)>&
Function<RetType(Args...)>::operator=(const Function& rhs) {
  if (storage_ptr_ != nullptr) {
    storage_ptr_->destroy();
    if (storage_ptr_ != reinterpret_cast<decltype(storage_ptr_)>(
                        std::addressof(stack_))) {
      ::operator delete(storage_ptr_);
    }
  }

  if (rhs.storage_ptr_ != nullptr) {
    if (rhs.storage_ptr_ == reinterpret_cast<decltype(rhs.storage_ptr_)>(
                            std::addressof(rhs.stack_))) {
      storage_ptr_ = reinterpret_cast<decltype(storage_ptr_)>(
                     std::addressof(stack_));
      rhs.storage_ptr_->construct(storage_ptr_);
    } else {
      storage_ptr_ = rhs.storage_ptr_->clone();
    }
  }

  return *this;
}

template <typename RetType, typename... Args>
RetType Function<RetType(Args...)>::operator()(Args&&... args) const {
  return storage_ptr_->operator()(std::forward<Args>(args)...);
}

template <typename RetType, typename... Args>
Function<RetType(Args...)>::operator bool() {
  return storage_ptr_ != nullptr;
}
// Function definition //-----------------------------------------------//

#endif // FUNCTION_H_
