#include <iostream>
#include <iterator>

template<size_t N>
class StackStorage {
 public:
  uint8_t buffer[N];
  uint8_t* empty_space = buffer;

  StackStorage() = default;
  StackStorage(const StackStorage&) = delete;
  StackStorage& operator=(const StackStorage&) = delete;

};
template<typename T, size_t N>
class StackAllocator {
 public:
  StackStorage<N>* allocator_strorage; 

  template<typename U>
  class rebind { 
   public:
    using other = StackAllocator<U, N>;
  };

  bool operator==(const StackAllocator& other) const { 
    return allocator_strorage == other.allocator_strorage;
  }

  bool operator!=(const StackAllocator& other) const {
    return !(*this == other);
  }

  using value_type = T; 

  StackAllocator() = delete; 

  ~StackAllocator() = default; 

  StackAllocator(StackStorage<N>& storage): allocator_strorage(&storage) {} 

  void deallocate(T*, size_t) {}; 

  template<typename U>
  StackAllocator(const StackAllocator<U, N>& other)
    : allocator_strorage(other.allocator_strorage) 
  {}

  template<typename U>
  StackAllocator<T, N>& operator=(StackAllocator<U, N>& other) {


    this->allocator_strorage = other.allocator_strorage;
    return *this;
  }

  T* allocate(size_t objects_number) {
    size_t current_position = 
        reinterpret_cast<size_t>(allocator_strorage->empty_space);

    size_t new_position = 
        (current_position + alignof(T) - 1) / alignof(T) * alignof(T);

    uint8_t* result = allocator_strorage->empty_space = 
        reinterpret_cast<uint8_t*>(new_position);

    allocator_strorage->empty_space += sizeof(T) * objects_number;
    return reinterpret_cast<T*>(result);
  }
};

template<typename T, typename Allocator = std::allocator<T>>
class List {
 private:

  struct BaseNode {
    BaseNode* next;
    BaseNode* prev;

    BaseNode(BaseNode* next_node, BaseNode* prev_node) {
      next = next_node;
      prev = prev_node;
    }

    BaseNode() {
      next = this;
      prev = this;
    }

    BaseNode(const BaseNode& other): next(other.next), prev(other.prev) {}
    ~BaseNode() = default;
  };
  struct Node: BaseNode {
    T value;

    Node(const T& value, const BaseNode& node)
      : BaseNode(node)
      , value(value)
    {}

    Node(const BaseNode& node)
      : BaseNode(node)
      , value() 
    {}

    Node(const Node& other)
      : BaseNode(other.next, other.prev)
      , value(other.value)
    {}

    Node(BaseNode* next, BaseNode* prev):BaseNode(next,prev),value(){}
    ~Node() = default;
  };

 private:

  using AllocTraits = std::allocator_traits<Allocator>;
  using NodeAllocator = typename AllocTraits::template rebind_alloc<Node>;
  using AllocatorTraits = std::allocator_traits<NodeAllocator>;

  size_t size_;
  NodeAllocator node_allocator_;
  BaseNode tail_;

 public:
  List();
  List(size_t);
  List(size_t, const T&);
  List(const Allocator&);
  List(size_t, Allocator&);
  List(size_t, const T&, Allocator&);

  void pop_back();
  void pop_front();
  void push_back(const T&);
  void push_back();
  void push_front(const T&);


  template<bool is_const>
  class base_iterator {
   private:
    using PointerType = 
        std::conditional_t<is_const, const BaseNode* , BaseNode*>;

    using ReferenceType = 
        std::conditional_t<is_const, const BaseNode&, BaseNode&>;

    PointerType node_;
   public:

    using value_type = T;
    using reference = std::conditional_t<is_const, const T&, T&>;
    using pointer = std::conditional_t<is_const, const T*, T*>;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = long long;

    base_iterator(): node_(nullptr) {}
    base_iterator(PointerType value): node_(value) {}
    base_iterator(const base_iterator<false>& other): node_(other.node_) {}
    //base_iterator<true>(const base_iterator<true>& other): node_(other.node_) {}
    base_iterator& operator++();
    base_iterator& operator--();
    base_iterator operator++(int);
    base_iterator operator--(int);
    base_iterator& operator=(const base_iterator&);
    typename std::conditional_t<is_const, const T&, T&> operator*();
    typename std::conditional_t<is_const, const T*, T*>operator->();
    bool operator==(const base_iterator) const;
    bool operator!=(const base_iterator) const;

    ReferenceType GetBaseNode() { return *node_; }
    PointerType GetBaseNodePtr() { return node_; }

    friend base_iterator<true>;
    friend base_iterator<false>;
  };

  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;
  
  reverse_iterator rbegin() { return std::make_reverse_iterator(end()); }
  reverse_iterator rend() { return std::make_reverse_iterator(begin()); }

  const_reverse_iterator rbegin() const { return std::make_reverse_iterator(end()); } 
  const_reverse_iterator rend() const { return std::make_reverse_iterator(begin()); } 
  const_reverse_iterator crbegin() const { return std::make_reverse_iterator(end()); } 
  const_reverse_iterator crend() const { return std::make_reverse_iterator(begin()); } 

  iterator insert(const_iterator, const T&);
  iterator insert(const_iterator);
  iterator erase(const_iterator);

  List(const List&);
  NodeAllocator get_allocator();
  size_t size() const { return size_; }
  ~List();
  void SwapBaseNodes(BaseNode&, BaseNode&);
  List& operator=(const List&);

};


template<typename T, typename Allocator>
List<T, Allocator>::List()
  : size_(0)
  , node_allocator_(NodeAllocator())
  , tail_(BaseNode())
{}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size): List() {
  for (size_t i = 0; i < size; ++i) {
    this->push_back();
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const T& value): List() {
  for (size_t i = 0; i < size; ++i) {
    this->push_back(value);
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const Allocator& alloc)
  : size_(0)
  , node_allocator_(alloc)
  , tail_(BaseNode())
{}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size, Allocator& alloc): List(alloc) {
  for (size_t i = 0; i < size; ++i) {
    this->push_back();
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const T& value, Allocator& alloc): List(alloc) {
  for (size_t i = 0; i < size; ++i) {
    this->push_back(value);
  }
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_back(const T& value) {
  this->insert(end(), value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_back() {
  this->insert(end());
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& value) {
  this->insert(begin(), value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::pop_back() {
  this->erase(--end());
}

template<typename T, typename Allocator>
void List<T, Allocator>::pop_front() {
  this->erase(begin());
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template base_iterator<is_const>&
List<T, Allocator>::base_iterator<is_const>::operator++() {
  this->node_ = this->node_->next;
  return *this;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template base_iterator<is_const>&
List<T, Allocator>::base_iterator<is_const>::operator--() {
  this->node_ = this->node_->prev;
  return *this;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template base_iterator<is_const> 
List<T, Allocator>::base_iterator<is_const>::operator++(int) {
  auto temp = *this;
  this->node_ = this->node_->next;
  return temp;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>::template base_iterator<is_const>
List<T, Allocator>::base_iterator<is_const>::operator--(int) {
  auto temp = *this;
  this->node_ = this->node_->prev;
  return temp;
}

template<typename T, typename Allocator>
template<bool is_const>
typename List<T, Allocator>:: template base_iterator<is_const>&
List<T, Allocator>::base_iterator<is_const>::operator=(const base_iterator& other) {
  auto temp = other;
  std::swap(temp.node_, this->node_);
  return *this;
}

template<typename T, typename Allocator>
template<bool is_const>
typename std::conditional_t<is_const, const T&, T&>
List<T, Allocator>::base_iterator<is_const>::operator*() {
  return (static_cast<std::conditional_t<is_const, const Node*, Node*>>(node_))->value;
}

template<typename T, typename Allocator>
template<bool is_const>
typename std::conditional_t<is_const, const T*, T*>
List<T, Allocator>::base_iterator<is_const>::operator->() {
  return &((static_cast<std::conditional_t<is_const, const Node*, Node*>>(node_))->value);
}

template<typename T, typename Allocator>
template<bool is_const>
bool List<T, Allocator>::base_iterator<is_const>::operator==(
    const base_iterator other) const {
  return node_ == other.node_;
}

template<typename T, typename Allocator>
template<bool is_const>
bool List<T, Allocator>::base_iterator<is_const>::operator!=(
    const base_iterator other) const {
  return !(*this == other);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator 
List<T, Allocator>::begin() {
  return ++iterator(&tail_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator 
List<T, Allocator>::end() {
  return iterator(&tail_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator 
List<T, Allocator>::begin() const {
  return ++const_iterator(&tail_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator 
List<T, Allocator>::end() const {
  return const_iterator(&tail_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator 
List<T, Allocator>::cbegin() const {
  return ++const_iterator(&tail_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::const_iterator 
List<T, Allocator>::cend() const {
  return const_iterator(&tail_);
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator 
List<T, Allocator>::insert(const_iterator position, const T& value) {

  BaseNode* next_node = const_cast<BaseNode*>(position.GetBaseNodePtr());
  BaseNode* prev_node = position.GetBaseNodePtr()->prev;

  Node* result = AllocatorTraits::allocate(node_allocator_, 1); 
  BaseNode current_base_node(next_node, prev_node);

  try {
    AllocatorTraits::construct(node_allocator_, result, value, current_base_node);
  } catch (...) {
    AllocatorTraits::deallocate(node_allocator_, result, 1);
    throw;
  }

  next_node->prev = static_cast<BaseNode*>(result);
  prev_node->next = static_cast<BaseNode*>(result);
  ++size_;
  return iterator(static_cast<BaseNode*>(result));
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator 
List<T, Allocator>::insert(const_iterator position) {

  BaseNode* next_node = const_cast<BaseNode*>(position.GetBaseNodePtr());
  BaseNode* prev_node = position.GetBaseNodePtr()->prev;

  Node* result = AllocatorTraits::allocate(node_allocator_, 1); 

  try {
    AllocatorTraits::construct(node_allocator_, result, next_node, prev_node);
  } catch (...) {
    AllocatorTraits::deallocate(node_allocator_, result, 1);
    throw;
  }

  next_node->prev = static_cast<BaseNode*>(result);
  prev_node->next = static_cast<BaseNode*>(result);
  ++size_;
  return iterator(static_cast<BaseNode*>(result));
}

template<typename T, typename Allocator>
typename List<T, Allocator>::iterator 
List<T, Allocator>::erase(const_iterator position) {
  if (size_ == 0) { return iterator(&tail_); }

  BaseNode* next_node = position.GetBaseNodePtr()->next;
  BaseNode* prev_node = position.GetBaseNodePtr()->prev;

  AllocatorTraits::destroy(node_allocator_, 
                           static_cast<const Node*>(position.GetBaseNodePtr()));

  AllocatorTraits::deallocate(node_allocator_, 
                              static_cast<typename AllocatorTraits::pointer>(
                                const_cast<BaseNode*>(position.GetBaseNodePtr())
                              ), 1);
  next_node->prev = prev_node;
  prev_node->next = next_node;
  --size_;
  return iterator(next_node);
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const List& other) 
  : size_(0)
  , node_allocator_(AllocatorTraits::select_on_container_copy_construction(other.node_allocator_))
  , tail_(BaseNode()) 
{
  for (const auto& i : other) {
    try {
      push_back(i);
    } catch (...) {
      while (begin() != end()) {
        erase(begin());
      }
      throw;
    }
  }
}

template<typename T, typename Allocator>
typename List<T, Allocator>::NodeAllocator 
List<T, Allocator>::get_allocator() { return node_allocator_; }

template<typename T, typename Allocator>
List<T, Allocator>::~List() {
  while (size_) {
    erase(begin());
  }
}

template<typename T, typename Allocator>
void
List<T, Allocator>::SwapBaseNodes(BaseNode& first, BaseNode& second) {
  BaseNode* first_next = first.next;
  BaseNode* first_prev = first.prev;
  BaseNode* second_next = second.next;
  BaseNode* second_prev = second.prev;

  first.next = second_next;
  first.prev = second_prev;

  second.next = first_next;
  second.prev = first_prev;

  first.next->prev = &first;
  first.prev->next = &first;

  second.next->prev = &second;
  second.prev->next = &second;
}

template<typename T, typename Allocator>
List<T, Allocator>& 
List<T, Allocator>::operator=(const List<T, Allocator>& other) {
  try {
    List<T, Allocator> temp(
        AllocatorTraits::propagate_on_container_copy_assignment::value ? 
        other.node_allocator_ :
        AllocatorTraits::select_on_container_copy_construction(other.node_allocator_));

    for (const auto& i : other) {
      temp.push_back(i);
    }
    SwapBaseNodes(temp.tail_, tail_); std::swap(size_, temp.size_); std::swap(node_allocator_, temp.node_allocator_);
    return *this;
  } catch (...) {
    throw;
  }
}
