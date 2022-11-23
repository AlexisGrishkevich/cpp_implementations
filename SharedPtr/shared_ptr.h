#pragma once
#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <exception>
#include <type_traits>
#include <memory>
#include <utility>


namespace details {

class bad_weak_ptr final: public std::exception {
  const char* what() const noexcept override {
    return "bad_weak_ptr";
  }
};


struct ControlBlockBase {
  std::size_t shared_counter_{0};
  std::size_t weak_counter_{0};

  virtual void destroy_field()         = 0;
  virtual void destroy_control_block() = 0;
  virtual ~ControlBlockBase()          = default;

  void increment_shared_counter() {
    ++shared_counter_;
  }

  void increment_weak_counter() {
    ++weak_counter_;
  }

  void decrement_shared_counter() {
    --shared_counter_;
  }

  void decrement_weak_counter() {
    --weak_counter_;
  }

  std::size_t get_shared_counter() {
    return shared_counter_;
  }

  std::size_t get_weak_counter() {
    return weak_counter_;
  }
};


template <typename T,
          typename Deleter = std::default_delete<T>,
          typename Allocator = std::allocator<T>>
struct ControlBlockConstructor final : public ControlBlockBase {
  T* ptr_{nullptr};
  Deleter deleter_{};
  Allocator alloc_{};

  ControlBlockConstructor(T* ptr, Deleter deleter = Deleter(),
    Allocator alloc = Allocator())
    : ptr_{ptr}, deleter_{std::move(deleter)}, alloc_{std::move(alloc)}
  { }

  void destroy_field() override {
      deleter_(ptr_);
      ptr_ = nullptr;
  }

  void destroy_control_block() override {
    using ControlBlockType = ControlBlockConstructor<T, Allocator, Deleter>;
    using ControlBlockAllocType = typename std::allocator_traits<Allocator>::
                                  template rebind_alloc<ControlBlockType>;
    ControlBlockAllocType rebind_alloc = std::move(alloc_);
    this->~ControlBlockConstructor();
    std::allocator_traits<ControlBlockAllocType>::deallocate(
                                                    rebind_alloc, this, 1);
  }

  ~ControlBlockConstructor() = default;
};


template <typename T, typename Allocator = std::allocator<T>>
struct ControlBlockAllocateShared final : public ControlBlockBase {
  alignas(T) char storage_[sizeof(T)];
  T* ptr_{nullptr};
  Allocator alloc_{};

  template <typename... Args>
  ControlBlockAllocateShared(Allocator alloc, Args&&... args)
      : ControlBlockBase{ }, alloc_{std::move(alloc)} {
    ptr_ = reinterpret_cast<T*>(storage_);
    std::allocator_traits<Allocator>::construct(alloc_, ptr_,
                                              std::forward<Args>(args)...);
  }

  void destroy_field() override {
    std::allocator_traits<Allocator>::destroy(alloc_, ptr_);
  }

  void destroy_control_block() override {
    using ControlBlockType = ControlBlockAllocateShared<T, Allocator>;
    using ControlBlockAllocType = typename std::allocator_traits<Allocator>::
                                  template rebind_alloc<ControlBlockType>;
    ControlBlockAllocType rebind_alloc = std::move(alloc_);
    this->~ControlBlockAllocateShared();
    std::allocator_traits<ControlBlockAllocType>::deallocate(
                                                    rebind_alloc, this, 1);
  }

  ~ControlBlockAllocateShared() = default;
};

} // namespace details


template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

template <typename T>
class EnableSharedFromThis;

template <typename T>
class SharedPtr final {
  template <typename U>
  friend class SharedPtr;
  template <typename U>
  friend class WeakPtr;
  template <typename U, typename Allocator, typename... Args>
  friend SharedPtr<U> allocateShared(const Allocator& alloc, Args&&... args);

  using ControlBlockBase = details::ControlBlockBase;

private:
  ControlBlockBase* control_block_{nullptr};
  T* ptr_{nullptr};

private:
  SharedPtr(ControlBlockBase* control_block, T* ptr);

public:
  constexpr SharedPtr() noexcept;
  template <typename U>
  explicit SharedPtr(U* ptr);
  template <typename Deleter>
  SharedPtr(T* ptr, Deleter deleter);
  template <typename U, typename Deleter, typename Allocator>
  SharedPtr(U* ptr, Deleter deleter, Allocator alloc);
  SharedPtr(const SharedPtr& rhs) noexcept;
  template <typename U>
  SharedPtr(const SharedPtr<U>& rhs) noexcept;
  SharedPtr(SharedPtr&& rhs) noexcept;
  template <typename U>
  SharedPtr(SharedPtr<U>&& rhs) noexcept;
  template <typename U>
  SharedPtr(const WeakPtr<U>& rhs);

  ~SharedPtr();

  SharedPtr& operator=(const SharedPtr& rhs) noexcept;
  template <typename U>
  SharedPtr& operator=(const SharedPtr<U>& rhs) noexcept;
  SharedPtr& operator=(SharedPtr&& rhs) noexcept;
  template <typename U>
  SharedPtr& operator=(SharedPtr<U>&& rhs) noexcept;

  T* get() const noexcept;
  std::size_t use_count() const noexcept;

  T& operator*() const noexcept;
  T* operator->() const noexcept;
  explicit operator bool() const noexcept;

  void swap(SharedPtr& rhs) noexcept;
  void reset() noexcept;
  template <typename U>
  void reset(U* ptr);
  template <typename U, typename Deleter, typename Allocator>
  void reset(U* ptr, Deleter deleter);
  template <typename U, typename Deleter, typename Allocator>
  void reset(U* ptr, Deleter deleter, Allocator alloc);
};

template <typename T, typename Allocator, typename... Args>
SharedPtr<T> allocateShared(const Allocator& alloc, Args&&... args);

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args);

template <typename T>
void swap(SharedPtr<T>& lhs, SharedPtr<T>& rhs) noexcept;
// SharedPtr declaration //---------------------------------------------//

// SharedPtr definition //----------------------------------------------//
template <typename T>
SharedPtr<T>::SharedPtr(ControlBlockBase* control_block, T* ptr)
    : control_block_{control_block}, ptr_{ptr} {
  if (control_block_)
    control_block_->increment_shared_counter();
}

template <typename T>
template <typename U, typename Deleter, typename Allocator>
SharedPtr<T>::SharedPtr(U* ptr, Deleter deleter, Allocator alloc)
    : ptr_{static_cast<T*>(ptr)} {
  using ControlBlockType = details::ControlBlockConstructor<
                                                    T, Allocator, Deleter>;
  using ControlBlockAllocType = typename std::allocator_traits<Allocator>::
                                template rebind_alloc<ControlBlockType>;
  ControlBlockAllocType rebind_alloc = alloc;
  auto tmp_ptr = std::allocator_traits<ControlBlockAllocType>::
                                                 allocate(rebind_alloc, 1);
  try {
    ::new (tmp_ptr) ControlBlockType(ptr, deleter, alloc);
    control_block_ = tmp_ptr;
    control_block_->increment_shared_counter();
  } catch(...) {
    tmp_ptr->destroy_field();
    tmp_ptr->destroy_control_block();
    deleter(ptr);
    throw;
  }
}

template <typename T>
constexpr SharedPtr<T>::SharedPtr() noexcept = default;

template <typename T>
template <typename U>
SharedPtr<T>::SharedPtr(U* ptr)
    : control_block_{new details::ControlBlockConstructor<U>(ptr)},
      ptr_{static_cast<T*>(ptr)} {
  if (control_block_)
    control_block_->increment_shared_counter();
}

template <typename T>
template <typename Deleter>
SharedPtr<T>::SharedPtr(T* ptr, Deleter deleter)
    : SharedPtr(ptr, deleter, std::allocator<T>())
{ }

template <typename T>
SharedPtr<T>::SharedPtr(const SharedPtr<T>& rhs) noexcept
    : control_block_{rhs.control_block_}, ptr_{rhs.ptr_} {
  if (control_block_)
    control_block_->increment_shared_counter();
}

template <typename T>
template <typename U>
SharedPtr<T>::SharedPtr(const SharedPtr<U>& rhs) noexcept
    : control_block_{rhs.control_block_}, ptr_{rhs.ptr_} {
  if (control_block_)
    control_block_->increment_shared_counter();
}

template <typename T>
SharedPtr<T>::SharedPtr(SharedPtr<T>&& rhs) noexcept
    : control_block_{rhs.control_block_}, ptr_{rhs.ptr_} {
  rhs.ptr_ = nullptr;
  rhs.control_block_ = nullptr;
}

template <typename T>
template <typename U>
SharedPtr<T>::SharedPtr(SharedPtr<U>&& rhs) noexcept
    : control_block_{rhs.control_block_}, ptr_{rhs.ptr_} {
  rhs.ptr_ = nullptr;
  rhs.control_block_ = nullptr;
}

template <typename T>
template <typename U>
SharedPtr<T>::SharedPtr(const WeakPtr<U>& rhs)
    : SharedPtr(rhs.control_block_, rhs.ptr_) {
  if (!control_block_)
    throw details::bad_weak_ptr();
}

template <typename T>
SharedPtr<T>::~SharedPtr() {
  if (!control_block_)
    return;

  control_block_->decrement_shared_counter();
  if (!(control_block_->get_shared_counter()))
    control_block_->destroy_field();

  if (!(control_block_->get_shared_counter()) &&
      !(control_block_->get_weak_counter())) {
    control_block_->destroy_control_block();
  }
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr& rhs) noexcept {
  SharedPtr(rhs).swap(*this);
  return *this;
}

template <typename T>
template <typename U>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr<U>& rhs) noexcept {
  SharedPtr(rhs).swap(*this);
  return *this;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr&& rhs) noexcept {
  SharedPtr(std::move(rhs)).swap(*this);
  return *this;
}

template <typename T>
template <typename U>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr<U>&& rhs) noexcept {
  SharedPtr(std::move(rhs)).swap(*this);
  return *this;
}

template <typename T>
T* SharedPtr<T>::get() const noexcept {
  return ptr_;
}

template <typename T>
std::size_t SharedPtr<T>::use_count() const noexcept {
  return control_block_ ? control_block_->get_shared_counter() : 0;
}

template <typename T>
T& SharedPtr<T>::operator*() const noexcept {
  return *ptr_;
}

template <typename T>
T* SharedPtr<T>::operator->() const noexcept {
  return ptr_;
}

template <typename T>
SharedPtr<T>::operator bool() const noexcept {
  return ptr_ != nullptr;
}

template <typename T>
void SharedPtr<T>::swap(SharedPtr<T>& rhs) noexcept {
  using std::swap;
  swap(ptr_, rhs.ptr_);
  swap(control_block_, rhs.control_block_);
}

template <typename T>
void SharedPtr<T>::reset() noexcept {
  SharedPtr<T>().swap(*this);
}

template <typename T>
template <typename U>
void SharedPtr<T>::reset(U* ptr) {
  SharedPtr<T>(ptr).swap(*this);
}

template <typename T>
template <typename U, typename Deleter, typename Allocator>
void SharedPtr<T>::reset(U* ptr, Deleter deleter) {
  SharedPtr<T>(ptr, std::move(deleter)).swap(*this);
}

template <typename T>
template <typename U, typename Deleter, typename Allocator>
void SharedPtr<T>::reset(U* ptr, Deleter deleter, Allocator alloc) {
  SharedPtr<T>(ptr, std::move(deleter), std::move(alloc)).swap(*this);
}

template <typename T, typename Allocator, typename... Args>
SharedPtr<T> allocateShared(const Allocator& alloc, Args&&... args) {
  using ControlBlockType = details::ControlBlockAllocateShared<T, Allocator>;
  using ControlBlockAllocType = typename std::allocator_traits<Allocator>::
                                template rebind_alloc<ControlBlockType>;
  ControlBlockAllocType rebind_alloc = alloc;
  auto tmp_ptr = std::allocator_traits<ControlBlockAllocType>::
                 allocate(rebind_alloc, 1);

  ::new (tmp_ptr) ControlBlockType(alloc, std::forward<Args>(args)...);
  SharedPtr<T> ans(tmp_ptr, tmp_ptr->ptr_);
  if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
    tmp_ptr->ptr_->wptr_ = ans;
  }

  return ans;
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
    return allocateShared<T, std::allocator<T>, Args...>(
      std::allocator<T>(), std::forward<Args>(args)...);
}

template <typename T>
void swap(SharedPtr<T>& lhs, SharedPtr<T>& rhs) noexcept {
  lhs.swap(rhs);
}
// SharedPtr definition //----------------------------------------------//

// WeakPtr declaration //-----------------------------------------------//
template <typename T>
class WeakPtr final {
  template <typename U>
  friend class WeakPtr;
  template <typename U>
  friend class SharedPtr;

  using ControlBlockBase = details::ControlBlockBase;

private:
  ControlBlockBase* control_block_{nullptr};
  T* ptr_{nullptr};

public:
  constexpr WeakPtr() noexcept;
  WeakPtr(const WeakPtr& rhs) noexcept;
  template <typename U>
  WeakPtr(const WeakPtr<U>& rhs) noexcept;
  template <typename U>
  WeakPtr(const SharedPtr<U>& rhs) noexcept;
  WeakPtr(WeakPtr&& rhs) noexcept;
  template <typename U>
  WeakPtr(WeakPtr<U>&& rhs) noexcept;

  ~WeakPtr();

  WeakPtr& operator=(const WeakPtr& rhs) noexcept;
  template <typename U>
  WeakPtr& operator=(const WeakPtr<U>& rhs) noexcept;
  template <typename U>
  WeakPtr& operator=(const SharedPtr<U>& rhs) noexcept;
  WeakPtr& operator=(WeakPtr&& rhs) noexcept;
  template <typename U>
  WeakPtr& operator=(WeakPtr<U>&& rhs) noexcept;

  std::size_t use_count() const noexcept;
  bool expired() const noexcept;
  void swap(WeakPtr& r) noexcept;
  void reset() noexcept;
  SharedPtr<T> lock() const;
};

template <typename T>
void swap(WeakPtr<T>& lhs, WeakPtr<T>& rhs) noexcept;
// WeakPtr declaration //-----------------------------------------------//

// WeakPtr definition //------------------------------------------------//
template <typename T>
constexpr WeakPtr<T>::WeakPtr() noexcept = default;

template <typename T>
WeakPtr<T>::WeakPtr(const WeakPtr<T>& rhs) noexcept
    : control_block_{rhs.control_block_}, ptr_{rhs.ptr_} {
  if (control_block_)
    control_block_->increment_weak_counter();
}

template <typename T>
template <typename U>
WeakPtr<T>::WeakPtr(const WeakPtr<U>& rhs) noexcept
    : control_block_{rhs.control_block_}, ptr_{rhs.ptr_} {
  if (control_block_)
    control_block_->increment_weak_counter();
}

template <typename T>
template <typename U>
WeakPtr<T>::WeakPtr(const SharedPtr<U>& rhs) noexcept 
    : control_block_{rhs.control_block_}, ptr_{rhs.ptr_} {
  if (control_block_)
    control_block_->increment_weak_counter();
}

template <typename T>
WeakPtr<T>::WeakPtr(WeakPtr<T>&& rhs) noexcept
    : control_block_{rhs.control_block_}, ptr_{rhs.ptr_} {
  rhs.ptr_ = nullptr;
  rhs.control_block_ = nullptr;
}

template <typename T>
template <typename U>
WeakPtr<T>::WeakPtr(WeakPtr<U>&& rhs) noexcept
    : control_block_{rhs.control_block_}, ptr_{rhs.ptr_} {
  rhs.ptr_ = nullptr;
  rhs.control_block_ = nullptr;
}

template <typename T>
WeakPtr<T>::~WeakPtr() {
  if (!control_block_)
    return;

  control_block_->decrement_weak_counter();
  if (!(control_block_->get_shared_counter()) &&
      !(control_block_->get_weak_counter()))
    control_block_->destroy_control_block();
}

template <typename T>
WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr<T>& rhs) noexcept {
  WeakPtr(rhs).swap(*this);
  return *this;
}

template <typename T>
template <typename U>
WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr<U>& rhs) noexcept {
  WeakPtr(rhs).swap(*this);
  return *this;
}

template <typename T>
template <typename U>
WeakPtr<T>& WeakPtr<T>::operator=(const SharedPtr<U>& rhs) noexcept {
  WeakPtr(rhs).swap(*this);
  return *this;
}

template <typename T>
WeakPtr<T>& WeakPtr<T>::operator=(WeakPtr<T>&& rhs) noexcept {
  WeakPtr(std::move(rhs)).swap(*this);
  return *this;
}

template <typename T>
template <typename U>
WeakPtr<T>& WeakPtr<T>::operator=(WeakPtr<U>&& rhs) noexcept {
  WeakPtr(std::move(rhs)).swap(*this);
  return *this;
}

template <typename T>
std::size_t WeakPtr<T>::use_count() const noexcept {
  return control_block_ ? control_block_->get_shared_counter() : 0;
}

template <typename T>
bool WeakPtr<T>::expired() const noexcept {
  return use_count() == 0;
}

template <typename T>
void WeakPtr<T>::swap(WeakPtr<T>& rhs) noexcept {
  using std::swap;
  swap(ptr_, rhs.ptr_);
  swap(control_block_, rhs.control_block_);
}

template <typename T>
void WeakPtr<T>::reset() noexcept{
  WeakPtr().swap(*this);
}

template <typename T>
SharedPtr<T> WeakPtr<T>::lock() const {
  return SharedPtr<T>(*this);
}

template <typename T>
void swap(WeakPtr<T>& lhs, WeakPtr<T>& rhs) noexcept {
  lhs.swap(rhs);
}
// WeakPtr definition //------------------------------------------------//


// EnableSharedFromThis declaration //----------------------------------//
template <typename T>
class EnableSharedFromThis {
  template <typename U>
  friend class SharedPtr;
  template <typename U, typename Allocator, typename... Args>
  friend SharedPtr<U> allocateShared(const Allocator& alloc, Args&&... args);

private:
  mutable WeakPtr<T> wptr_{};

protected:
  constexpr EnableSharedFromThis() noexcept;
  EnableSharedFromThis(const EnableSharedFromThis& rhs) noexcept;
  EnableSharedFromThis& operator=(const EnableSharedFromThis& rhs) noexcept;
  ~EnableSharedFromThis();

public:
  SharedPtr<T> shared_from_this();
  WeakPtr<T> weak_from_this() noexcept;
};
// EnableSharedFromThis declaration //----------------------------------//

// EnableSharedFromThis definition //-----------------------------------//
template <typename T>
constexpr EnableSharedFromThis<T>::EnableSharedFromThis() noexcept = default;

template <typename T>
EnableSharedFromThis<T>::EnableSharedFromThis(const EnableSharedFromThis& rhs) noexcept
{ }

template <typename T>
EnableSharedFromThis<T>&
EnableSharedFromThis<T>::operator=(const EnableSharedFromThis& rhs) noexcept {
  return *this;
}

template <typename T>
EnableSharedFromThis<T>::~EnableSharedFromThis()
{ }

template <typename T>
SharedPtr<T> EnableSharedFromThis<T>::shared_from_this() {
  return wptr_.lock();
}

template <typename T>
WeakPtr<T> EnableSharedFromThis<T>::weak_from_this() noexcept {
  return wptr_;
}
// EnableSharedFromThis definition //-----------------------------------//

#endif // SHARED_PTR_H
