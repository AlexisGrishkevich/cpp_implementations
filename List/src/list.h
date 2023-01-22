#pragma once
#ifndef LIST_H_
#define LIST_H_

#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>


#include <iostream>

template <typename T, typename Allocator = std::allocator<T>>
class List final {
  static_assert(std::is_same_v<std::remove_cv_t<T>, T>,
    "This constainer (list) must have a non-const, non-volatile value_type");

  static_assert(std::is_same_v<typename Allocator::value_type, T>,
    "This constainer (list) must have the same value_type as its allocator");

private:
  struct BaseNode {
  public:
    BaseNode* next_{nullptr};
    BaseNode* prev_{nullptr};

  public:
    static void swap(BaseNode& lhs, BaseNode& rhs) noexcept {
      std::swap(lhs, rhs);

      if (lhs.next_ == &rhs) {
        lhs.make_loop();
      } else {
        lhs.next_->prev_ = lhs.prev_->next_ = &lhs;
      }

      if (rhs.next_ == &lhs) {
        rhs.make_loop();
      } else {
        rhs.next_->prev_ = rhs.prev_->next_ = &rhs;
      }
    }

    void make_loop() noexcept {
      next_ = prev_ = this;
    }
  };

  struct Node : public BaseNode {
  public:
    T value_{};

  public:
    Node() = default;

    template <typename... Args>
    Node(Args&&... args)
        : BaseNode(), value_(std::forward<Args>(args)...)
    { }
  };

  template <bool IsConst>
  struct CommonIterator;

public:
  using value_type            = T;
  using pointer               = T*;
  using const_pointer         = const T*;
  using reference             = T&;
  using const_reference       = const T&;
  using size_type             = std::size_t;
  using difference_type       = std::ptrdiff_t;
  using iterator              = CommonIterator<false>;
  using const_iterator        = CommonIterator<true>;
  using reverse_itertor       = std::reverse_iterator<iterator>;
  using const_reverse_itertor = std::reverse_iterator<const_iterator>;
  using AllocType             = Allocator;
  using AllocTraits           = std::allocator_traits<Allocator>;
  using NodeAllocType         = typename std::allocator_traits<Allocator>::
                                template rebind_alloc<Node>;
  using NodeAllocTraits       = std::allocator_traits<NodeAllocType>;

private:
  BaseNode* fake_node_{nullptr};
  std::size_t size_{0};
  NodeAllocType alloc_{};

private:
  template <typename... Args>
  Node* create_node(Args&&... args);
  void destroy_node(Node* node) noexcept;
  void default_initialize();
  void increment_size(std::size_t count = 1) noexcept;
  void decrement_size(std::size_t count = 1) noexcept;
  void set_size(std::size_t count) noexcept;
  void check_container_is_empty();

public:
  explicit List(const Allocator& alloc = Allocator());
  explicit List(size_type count, const T& value = T(),
                const Allocator& alloc = Allocator());
  template <typename InputIt,
            typename = std::enable_if_t<std::is_class_v<InputIt>>>
  List(InputIt first, InputIt last, const Allocator& alloc = Allocator());
  List(std::initializer_list<T> ilist, const Allocator& alloc = Allocator());
  List(const List& rhs);
  List(List&& rhs);

  ~List();

  List& operator=(const List& rhs);
  List& operator=(List&& rhs);
  List& operator=(std::initializer_list<T> ilist);

  template <typename InputIt>
  void assign(InputIt first, InputIt last);

  Allocator get_allocator() const noexcept;
  size_type size() const noexcept;
  [[nodiscard]] bool empty() const noexcept;

  iterator begin() noexcept;
  const_iterator begin() const noexcept;
  iterator end() noexcept;
  const_iterator end() const noexcept;
  const_iterator cbegin() const noexcept;
  const_iterator cend() const noexcept;
  reverse_itertor rbegin() noexcept;
  const_reverse_itertor rbegin() const noexcept;
  reverse_itertor rend() noexcept;
  const_reverse_itertor rend() const noexcept;
  const_reverse_itertor crbegin() const noexcept;
  const_reverse_itertor crend() const noexcept;

  reference front();
  const_reference front() const;
  reference  back();
  const_reference back() const;

  template <typename... Args>
  iterator emplace(const_iterator pos, Args&&... args);
  iterator erase(const_iterator pos);

  void clear();
  iterator insert(const_iterator pos, const T& value);
  iterator insert(const_iterator pos, T&& value);
  void push_back(const T& value);
  void push_back(T&& value);
  template <typename... Args>
  reference emplace_back(Args&&... args);
  void pop_back();
  void push_front(const T& value);
  void push_front(T&& value);
  template <typename... Args>
  reference emplace_front(Args&&... args);
  void pop_front();
  void resize(std::size_t new_size, const T& value = T());

  void splice(const_iterator pos, List&& rhs);
  void splice(const_iterator pos, List& rhs);
  void swap(List& rhs) noexcept(AllocTraits::is_always_equal::value);
};

template <typename T, typename Allocator>
void swap(List<T, Allocator>& lhs, List<T, Allocator>& rhs)
  noexcept(noexcept(lhs.swap(rhs)));
// List declaratoin //--------------------------------------------------//

// CommonIterator declaration //----------------------------------------//
template <typename T, typename Allocator>
template <bool IsConst>
struct List<T, Allocator>::CommonIterator final {
public:
  using difference_type   = std::ptrdiff_t;
  using value_type        = T;
  using pointer           = std::conditional_t<IsConst, const T*, T>;
  using reference         = std::conditional_t<IsConst, const T&, T&>;
  using iterator_category = std::bidirectional_iterator_tag;

public:
  BaseNode* node_{nullptr};

public:
  CommonIterator() = default;
  CommonIterator(BaseNode* node);
  CommonIterator(const CommonIterator& rhs);
  CommonIterator& operator=(const CommonIterator& rhs);

  reference operator*() const;
  pointer operator->() const;
  CommonIterator& operator++();
  CommonIterator operator++(int);
  CommonIterator& operator--();
  CommonIterator operator--(int);
  operator CommonIterator<true>();
  bool operator==(const CommonIterator& rhs) const;
  bool operator!=(const CommonIterator& rhs) const;

  void swap(CommonIterator& rhs);
};

// CommonIterator definition //-----------------------------------------//
template <typename T, typename Allocator>
template <bool IsConst>
List<T, Allocator>::CommonIterator<IsConst>::
CommonIterator(BaseNode* node) : node_{node}
{ }

template <typename T, typename Allocator>
template <bool IsConst>
List<T, Allocator>::CommonIterator<IsConst>::
CommonIterator(const CommonIterator<IsConst>& rhs) : node_{rhs.node_}
{ }

template <typename T, typename Allocator>
template <bool IsConst>
typename List<T, Allocator>::template CommonIterator<IsConst>&
List<T, Allocator>::CommonIterator<IsConst>::
operator=(const CommonIterator<IsConst>& rhs) {
  node_ = rhs.node_;
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename List<T, Allocator>::template CommonIterator<IsConst>::reference
List<T, Allocator>::CommonIterator<IsConst>::operator*() const {
  return static_cast<Node*>(node_)->value_;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename List<T, Allocator>::template CommonIterator<IsConst>::pointer
List<T, Allocator>::CommonIterator<IsConst>::operator->() const {
  return std::addressof(static_cast<Node*>(node_)->value_);
}

template <typename T, typename Allocator>
template <bool IsConst>
typename List<T, Allocator>::template CommonIterator<IsConst>&
List<T, Allocator>::CommonIterator<IsConst>::operator++() {
  node_ = node_->next_;
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename List<T, Allocator>::template CommonIterator<IsConst>
List<T, Allocator>::CommonIterator<IsConst>::operator++(int) {
  auto tmp{*this};
  ++(*this);
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename List<T, Allocator>::template CommonIterator<IsConst>&
List<T, Allocator>::CommonIterator<IsConst>::operator--() {
  node_ = node_->prev_;
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename List<T, Allocator>::template CommonIterator<IsConst>
List<T, Allocator>::CommonIterator<IsConst>::operator--(int) {
  auto tmp{*this};
  --(*this);
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
List<T, Allocator>::CommonIterator<IsConst>::
operator CommonIterator<true>() {
  return CommonIterator<true>(node_);
}

template <typename T, typename Allocator>
template <bool IsConst>
bool List<T, Allocator>::CommonIterator<IsConst>::
operator==(const CommonIterator& rhs) const {
  return node_ == rhs.node_;
}

template <typename T, typename Allocator>
template <bool IsConst>
bool List<T, Allocator>::CommonIterator<IsConst>::
operator!=(const CommonIterator& rhs) const {
  return node_ != rhs.node_;
}

template <typename T, typename Allocator>
template <bool IsConst>
void List<T, Allocator>::CommonIterator<IsConst>::swap(CommonIterator& rhs) {
  BaseNode::swap(node_, rhs.node_);
}
// CommonIterator definition //-----------------------------------------//

// List definition //---------------------------------------------------//
template <typename T, typename Allocator>
template <typename... Args>
typename List<T, Allocator>::Node*
List<T, Allocator>::create_node(Args&&... args) {
  Node* new_node = NodeAllocTraits::allocate(alloc_, 1);

  try {
    NodeAllocTraits::construct(alloc_, new_node, std::forward<Args>(args)...);
  } catch (...) {
    NodeAllocTraits::deallocate(alloc_, new_node, 1);
    throw;
  }

  return new_node;
}

template <typename T, typename Allocator>
void List<T, Allocator>::destroy_node(Node* node) noexcept {
  NodeAllocTraits::destroy(alloc_, node);
  NodeAllocTraits::deallocate(alloc_, node, 1);
}

template <typename T, typename Allocator>
void List<T, Allocator>::default_initialize() {
  auto tmp_node = NodeAllocTraits::allocate(alloc_, 1);
  fake_node_ = static_cast<BaseNode*>(tmp_node);
  fake_node_->make_loop();
  set_size(0);
}

template <typename T, typename Allocator>
void List<T, Allocator>::increment_size(std::size_t count) noexcept {
  size_ += count;
}

template <typename T, typename Allocator>
void List<T, Allocator>::decrement_size(std::size_t count) noexcept {
  size_ -= count;
}

template <typename T, typename Allocator>
void List<T, Allocator>::set_size(std::size_t count) noexcept {
  size_ = count;
}

template <typename T, typename Allocator>
void List<T, Allocator>::check_container_is_empty() {
  if (empty())
    throw std::runtime_error{"Invalid operation (List is empty)."};
}

template <typename T, typename Allocator>
List<T, Allocator>::List(const Allocator& alloc)
    : alloc_(NodeAllocType(alloc)) {
  default_initialize();
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_type count, const T& value,
    const Allocator& alloc) : List(NodeAllocType(alloc)) {
  resize(count, value);
}

template <typename T, typename Allocator>
template <typename InputIt, typename>
List<T, Allocator>::List(InputIt first, InputIt last,
    const Allocator& alloc) : List(NodeAllocType(alloc)) {
  assign(first, last);
}

template <typename T, typename Allocator>
List<T, Allocator>::List(std::initializer_list<T> ilist,
    const Allocator& alloc) : List(NodeAllocType(alloc)) {
  assign(ilist.begin(), ilist.end());
}

template <typename T, typename Allocator>
List<T, Allocator>::List(const List& rhs)
    : List(NodeAllocTraits::select_on_container_copy_construction(
           rhs.get_allocator())) {
  assign(rhs.begin(), rhs.end());
}

template <typename T, typename Allocator>
List<T, Allocator>::List(List&& rhs)
    : fake_node_(rhs.fake_node_), size_(rhs.size_),
      alloc_(std::move(rhs.alloc_)) {
  rhs.default_initialize();
}

template <typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(const List& rhs) {
  if (this == &rhs)
    return *this;

  if (NodeAllocTraits::propagate_on_container_copy_assignment::value) {
    List tmp(rhs);
    swap(tmp);
  } else if (NodeAllocTraits::is_always_equal::value || alloc_ == rhs.alloc_) {
    List tmp(rhs);
    swap(tmp);
  } else {
    assign(rhs.begin(), rhs.end());
  }

  return *this;
}

template <typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(List&& rhs) {
  if (this == &rhs)
    return *this;

  if (NodeAllocTraits::propagate_on_container_move_assignment::value) {
    clear();
    NodeAllocTraits::deallocate(alloc_, static_cast<Node*>(fake_node_), 1);
    alloc_ = std::move(rhs.alloc_);
    fake_node_ = std::exchange(rhs.fake_node_, nullptr);
    size_ = std::exchange(rhs.size_, 0);
    rhs.default_initialize();
  } else if (NodeAllocTraits::is_always_equal::value || alloc_ == rhs.alloc_) {
    clear();
    NodeAllocTraits::deallocate(alloc_, static_cast<Node*>(fake_node_), 1);
    fake_node_ = std::exchange(rhs.fake_node_, nullptr);
    size_ = std::exchange(rhs.size_, 0);
    rhs.default_initialize();
  } else {
    assign(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
  }

  return *this;
}

template <typename T, typename Allocator>
List<T, Allocator>&
List<T, Allocator>::operator=(std::initializer_list<T> ilist) {
  assign(ilist.begin(), ilist.end());
  return *this;
}

template <typename T, typename Allocator>
List<T, Allocator>::~List() {
  clear();
  NodeAllocTraits::deallocate(alloc_, static_cast<Node*>(fake_node_), 1);
}

template <typename T, typename Allocator>
template <typename InputIt>
void List<T, Allocator>::assign(InputIt first, InputIt last) {
  clear();
  for (; first != last; ++first)
    emplace_back(*first);
}

template <typename T, typename Allocator>
Allocator List<T, Allocator>::get_allocator() const noexcept {
  return Allocator(alloc_);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::size_type
List<T, Allocator>::size() const noexcept {
  return size_;
}

template <typename T, typename Allocator>
[[nodiscard]] bool List<T, Allocator>::empty() const noexcept {
  return !size_;
}

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator
List<T, Allocator>::begin() noexcept {
  return iterator(fake_node_->next_);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_iterator
List<T, Allocator>::begin() const noexcept {
  return cbegin();
}

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator
List<T, Allocator>::end() noexcept {
  return iterator(fake_node_);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_iterator
List<T, Allocator>::end() const noexcept {
  return cend();
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_iterator
List<T, Allocator>::cbegin() const noexcept {
  return const_iterator(fake_node_->next_);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_iterator
List<T, Allocator>::cend() const noexcept {
  return const_iterator(fake_node_);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::reverse_itertor
List<T, Allocator>::rbegin() noexcept {
  return reverse_itertor(end());
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_itertor
List<T, Allocator>::rbegin() const noexcept {
  return crbegin();
}

template <typename T, typename Allocator>
typename List<T, Allocator>::reverse_itertor
List<T, Allocator>::rend() noexcept {
  return reverse_itertor(begin());
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_itertor
List<T, Allocator>::rend() const noexcept {
  return crend();
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_itertor
List<T, Allocator>::crbegin() const noexcept {
  return const_reverse_itertor(cend());
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_itertor
List<T, Allocator>::crend() const noexcept {
  return const_reverse_itertor(cbegin());
}

template <typename T, typename Allocator>
typename List<T, Allocator>::reference
List<T, Allocator>::front() {
  check_container_is_empty();
  return *begin();
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_reference
List<T, Allocator>::front() const {
  check_container_is_empty();
  return *cbegin();
}

template <typename T, typename Allocator>
typename List<T, Allocator>::reference
List<T, Allocator>::back() {
  check_container_is_empty();
  auto tmp = end();
  --tmp;
  return *tmp;
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_reference
List<T, Allocator>::back() const {
  check_container_is_empty();
  auto tmp = cend();
  --tmp;
  return *tmp;
}

template <typename T, typename Allocator>
template <typename... Args>
typename List<T, Allocator>::iterator
List<T, Allocator>::emplace(const_iterator pos, Args&&... args) {
  auto new_node = create_node(std::forward<Args>(args)...);
  new_node->next_ = pos.node_;
  new_node->prev_ = pos.node_->prev_;
  pos.node_->prev_ = pos.node_->prev_->next_ = new_node;
  increment_size();
  return iterator(static_cast<BaseNode*>(new_node));
}

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator
List<T, Allocator>::erase(const_iterator pos) {
  check_container_is_empty();
  auto old_node = pos.node_;
  auto return_node = old_node->next_;
  pos.node_->next_->prev_ = pos.node_->prev_;
  pos.node_->prev_->next_ = old_node->next_;
  destroy_node(static_cast<Node*>(old_node));
  decrement_size();
  return iterator(return_node);
}

template <typename T, typename Allocator>
void List<T, Allocator>::clear() {
  while (!empty())
    pop_back();
}

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator
List<T, Allocator>::insert(const_iterator pos, const T& value) {
  return emplace(pos, value);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator
List<T, Allocator>::insert(const_iterator pos, T&& value) {
  return emplace(pos, std::move(value));
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_back(const T& value) {
  insert(end(), value);
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_back(T&& value) {
  insert(end(), std::move(value));
}

template <typename T, typename Allocator>
template <typename... Args>
typename List<T, Allocator>::reference
List<T, Allocator>::emplace_back(Args&&... args) {
  emplace(end(), std::forward<Args>(args)...);
  return back();
}

template <typename T, typename Allocator>
void List<T, Allocator>::pop_back() {
  erase(iterator(fake_node_->prev_));
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& value) {
  insert(begin(), value);
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_front(T&& value) {
  insert(begin(), std::move(value));
}

template <typename T, typename Allocator>
template <typename... Args>
typename List<T, Allocator>::reference
List<T, Allocator>::emplace_front(Args&&... args) {
  emplace(begin(), std::forward<Args>(args)...);
  return front();
}

template <typename T, typename Allocator>
void List<T, Allocator>::pop_front() {
  erase(begin());
}

template <typename T, typename Allocator>
void List<T, Allocator>::resize(std::size_t new_size, const T& value) {
  while (size_ < new_size)
    push_back(value);

  while (size_ > new_size)
    pop_back();
}

template <typename T, typename Allocator>
void List<T, Allocator>::splice(const_iterator pos, List&& rhs) {
  if (alloc_ == rhs.alloc_) {
    rhs.fake_node_->next_->prev_ = pos.node_->prev_;
    pos.node_->prev_->next_ = rhs.fake_node_->next_;
    rhs.fake_node_->prev_->next_ = pos.node_;
    pos.node_->prev_ = rhs.fake_node_->prev_;
    increment_size(rhs.size_);
    rhs.fake_node_->make_loop();
    rhs.set_size(0);
  } else {
    for (auto&& item : rhs) {
      push_back(item);
    }

    rhs.clear();
  }
}

template <typename T, typename Allocator>
void List<T, Allocator>::splice(const_iterator pos, List& rhs) {
  splice(pos, std::move(rhs));
}

template <typename T, typename Allocator>
void List<T, Allocator>::swap(List& rhs) noexcept(AllocTraits::
                                                  is_always_equal::value) {
  BaseNode::swap(*fake_node_, *rhs.fake_node_);
  std::swap(size_, rhs.size_);
  if (NodeAllocTraits::propagate_on_container_swap::value) {
    std::swap(alloc_, rhs.alloc_);
  }
}

template <typename T, typename Allocator>
void swap(List<T, Allocator>& lhs, List<T, Allocator>& rhs)
    noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}
// List definition //---------------------------------------------------//

#endif // LIST_H_
