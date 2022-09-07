#include <iostream>
#include <memory>
#include <optional>


template <typename T, typename Allocator = std::allocator<T>>
struct SpecialDeleter {

  SpecialDeleter() = default;

  void operator()(T* a) const {
    Allocator alloc;
    std::allocator_traits<Allocator>::destroy(alloc, a);
    std::allocator_traits<Allocator>::deallocate(alloc, a, 1);
  }
};

struct ControlBlockBase {
  virtual size_t& getSharedCounter() = 0;
  virtual size_t& getWeakCounter() = 0;
  virtual void killControlBlock() = 0;
  virtual void deleter(void*) = 0;
  virtual void destroy_object() = 0;
};

template<typename U, typename Deleter = SpecialDeleter<U, std::allocator<U>> ,typename Allocator = std::allocator<U> >
class ControlBlock : public ControlBlockBase {
 public:
  size_t sharedCounter;
  size_t weakCounter;
  Allocator uAlloc;
  Deleter uDeleter;

  using CBType = ControlBlock<U, Deleter, Allocator>;
  using CBAlloc = typename std::allocator_traits<Allocator>::template rebind_alloc<CBType>;
  using CBAllocTraits = std::allocator_traits<CBAlloc>;

  void deleter(void* ptr) override {
    uDeleter.operator()(static_cast<U*>(ptr));
  }

  void destroy_object() override {}

  ControlBlock() :
      sharedCounter(1), weakCounter(0), uAlloc(), uDeleter() {}

  ControlBlock(const Deleter& deleter)
      : sharedCounter(1), weakCounter(0), uAlloc(), uDeleter(deleter) {}

  ControlBlock(const Deleter& deleter, const Allocator& allocator)
      : sharedCounter(1), weakCounter(0), uAlloc(allocator), uDeleter(deleter) {}

  size_t& getSharedCounter() override {
    return sharedCounter;
  }

  size_t& getWeakCounter() override {
    return weakCounter;
  }

  void killControlBlock() override {
    CBAlloc copy(uAlloc);
    CBAllocTraits::deallocate(copy, this, 1);
  }
};


template<typename U, typename Allocator = std::allocator<U>>
class ControlBlockWithObject : public ControlBlock<U, SpecialDeleter<U, Allocator>, Allocator> {
 public:
  using CBType = ControlBlockWithObject<U, Allocator>;
  using CBAlloc = typename std::allocator_traits<Allocator>::template rebind_alloc<CBType>;
  using CBAllocTraits = std::allocator_traits<CBAlloc>;
  U object;

  void destroy_object() override {
    std::allocator_traits<Allocator>::destroy(ControlBlock<U, SpecialDeleter<U, Allocator>, Allocator>::uAlloc, &object);
  }

  template <typename... Args>
  ControlBlockWithObject(const Allocator& allocator, Args&&... args)
      :ControlBlock<U, SpecialDeleter<U, Allocator>, Allocator>(SpecialDeleter<U, Allocator>(), allocator),
       object(std::forward<Args>(args)...) {}

  void killControlBlock() override {
    CBAlloc copy(ControlBlock<U, SpecialDeleter<U, Allocator>, Allocator>::uAlloc);
    ControlBlockWithObject<U, Allocator>::CBAllocTraits::deallocate(copy, this, 1);
  }

 private:
  ~ControlBlockWithObject() {}
};

template <typename U>
class WeakPtr;

template <typename T>
class SharedPtr {
  template<typename U, typename... Args>
  friend SharedPtr<U> makeShared(Args&&...);

  template <typename U, typename Allocator, typename... Args>
  friend SharedPtr<U> allocateShared(Allocator, Args&&...);

  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;

 private:
  T* ptr = nullptr;
  ControlBlockBase* cptr = nullptr;

  template <typename U>
  SharedPtr(const WeakPtr<U>& other) : ptr(other.ptr), cptr(other.cptr) {
    if (cptr)
      cptr->getSharedCounter() += 1;
  }


 public:
  template <typename Y, typename Allocator = std::allocator<Y>>
  explicit SharedPtr(ControlBlockWithObject<Y, Allocator>* cptr) : cptr(cptr) {}

  T* get() const {
    if (ptr)
      return ptr;
    return reinterpret_cast<T*>(cptr);
  }

  void swap(SharedPtr& other) {
    std::swap(ptr, other.ptr);
    std::swap(cptr, other.cptr);
  }

  SharedPtr() : ptr(nullptr), cptr(nullptr) {}

  template<class Y>
  explicit SharedPtr(Y* ptr_) : ptr(ptr_) {
    using CBType = ControlBlock<Y, SpecialDeleter<Y>, std::allocator<Y> >;
    using CBAllocTraits = std::allocator_traits<std::allocator<CBType>>;
    std::allocator<CBType> alloc;
    auto mem = CBAllocTraits::allocate(alloc, 1);
    CBAllocTraits::construct(alloc, mem);
    cptr = mem;
  }

  template<class Y, class Deleter>
  SharedPtr(Y* ptr_, Deleter d) : ptr(ptr_) {
    using CBType = ControlBlock<Y, Deleter, std::allocator<Y> >;
    using CBAllocTraits = std::allocator_traits<std::allocator<CBType>>;
    std::allocator<CBType> alloc;
    auto mem = CBAllocTraits::allocate(alloc, 1);
    CBAllocTraits::construct(alloc, mem, d);
    cptr = mem;
  }

  template<class Y, class Deleter, class Allocator>
  SharedPtr(Y* ptr_, Deleter d, Allocator allocator) : ptr(ptr_) {
    using CBType = ControlBlock<Y, Deleter, Allocator>;
    using CBAlloc = typename std::allocator_traits<Allocator>::template rebind_alloc<CBType>;
    using CBAllocTraits = std::allocator_traits<CBAlloc>;
    CBAlloc alloc(allocator);
    auto mem = CBAllocTraits::allocate(alloc, 1);
    //CBAllocTraits::construct(alloc, mem, d, alloc);
    cptr = mem;
    new(cptr) CBType(d, alloc);
  }

  template<typename U>
  SharedPtr(const SharedPtr<U>& other) : ptr(other.ptr), cptr(other.cptr) {
    if (cptr)
      cptr->getSharedCounter() += 1;
  }

  SharedPtr(const SharedPtr<T>& other) : ptr(other.ptr), cptr(other.cptr) {
    if (cptr)
      cptr->getSharedCounter() += 1;
  }

  template<typename U>
  SharedPtr& operator=(const SharedPtr<U>& other) {
    SharedPtr copy = other;
    this->swap(copy);
    return *this;
  }

  SharedPtr& operator=(const SharedPtr<T>& other) {
    SharedPtr copy = other;
    this->swap(copy);
    return *this;
  }

  template <typename U>
  SharedPtr(SharedPtr<U>&& other) : ptr(other.ptr), cptr(other.cptr) {
    other.ptr = nullptr;
    other.cptr = nullptr;
  }

  SharedPtr(SharedPtr<T>&& other) : ptr(other.ptr), cptr(other.cptr) {
    other.ptr = nullptr;
    other.cptr = nullptr;
  }

  template <typename U>
  SharedPtr& operator=(SharedPtr<U>&& other) {
    SharedPtr move_copy = std::move(other);
    this->swap(move_copy);
    return *this;
  }

  SharedPtr& operator=(SharedPtr<T>&& other) {
    SharedPtr move_copy = std::move(other);
    this->swap(move_copy);
    return *this;
  }

  T& operator*() const {
    if (ptr)
      return *ptr;
    return static_cast<ControlBlockWithObject<T>*>(cptr)->object;
  }

  T* operator->() const {
    if (ptr)
      return ptr;
    return &(static_cast<ControlBlockWithObject<T>*>(cptr)->object);
  }

  size_t use_count() const {
    if (cptr)
      return cptr->getSharedCounter();
    return 0;
  }

  ~SharedPtr() {
    if (!cptr)
      return;
    --(cptr->getSharedCounter());
    if (use_count() > 0)
      return;

    if (ptr) {
      cptr->deleter(ptr);
    } else {
      cptr->destroy_object();
    }

    if (cptr->getWeakCounter() == 0) {
      cptr->killControlBlock();
      cptr = nullptr;
    }
  }


  void reset() {
    SharedPtr<T>().swap(*this);
  }

  template <typename Y>
  void reset(Y* ptr) {
    SharedPtr<T>(ptr).swap(*this);
  }

  template <typename Y, typename Deleter>
  void reset(Y* ptr, Deleter deleter) {
    SharedPtr<T>(ptr, deleter).swap(*this);
  }

  template <typename Y, typename Deleter, typename Allocator>
  void reset(Y* ptr, Deleter deleter, Allocator allocator) {
    SharedPtr<T>(ptr, deleter, allocator).swap(*this);
  }
};




template <typename T, typename Allocator = std::allocator<T>, typename... Args>
SharedPtr<T> allocateShared(Allocator alloc, Args&&... args) {
  using CBType = ControlBlockWithObject<T, Allocator>;
  using CBAlloc = typename std::allocator_traits<Allocator>::template rebind_alloc<CBType>;
  using CBAllocTraits = std::allocator_traits<CBAlloc>;

  CBAlloc localAlloc(alloc);

  ControlBlockWithObject<T, Allocator>* mem = CBAllocTraits::allocate(localAlloc, 1);

  CBAllocTraits::construct(localAlloc, mem, alloc, std::forward<Args>(args)...);

  return SharedPtr<T>(mem);
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  return allocateShared<T>(std::allocator<T>(), std::forward<Args>(args)...);
}
template <typename T>
class WeakPtr {
  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;

 private:

  T* ptr = nullptr;
  ControlBlockBase* cptr = nullptr;

 public:

  WeakPtr() : ptr(nullptr), cptr(nullptr) {}

  template <typename U>
  WeakPtr(const SharedPtr<U>& a) : ptr(a.ptr), cptr(a.cptr) {
    if (cptr)
      cptr->getWeakCounter() += 1;
  }

  WeakPtr(const SharedPtr<T>& a) : ptr(a.ptr), cptr(a.cptr) {
    if (cptr) {
      cptr->getWeakCounter() += 1;
    }
  }

  template <typename U>
  WeakPtr(const WeakPtr<U>& a) : ptr(a.ptr), cptr(a.cptr) {
    if (cptr)
      cptr->getWeakCounter() += 1;
  }

  WeakPtr(const WeakPtr<T>& a) : ptr(a.ptr), cptr(a.cptr) {
    if (cptr)
      cptr->getWeakCounter() += 1;
  }

  void swap(WeakPtr& a) {
    std::swap(ptr, a.ptr);
    std::swap(cptr, a.cptr);
  }

  template<typename U>
  WeakPtr& operator=(const SharedPtr<U>& other) {
    WeakPtr copy = other;
    swap(copy);
    return *this;
  }

  WeakPtr& operator=(const SharedPtr<T>& other) {
    WeakPtr copy = other;
    swap(copy);
    return *this;
  }

  size_t use_count() const {
    if (cptr) {
      return cptr->getSharedCounter();
    }
    return 0;
  }

  SharedPtr<T> lock() const {
    if (expired())
      return SharedPtr<T>();
    return SharedPtr<T>(*this);
  }

  bool expired() const {
    return use_count() == 0;
  }

  ~WeakPtr() {
    if (!cptr)
      return;

    --(cptr->getWeakCounter());
    if (cptr->getWeakCounter() > 0)
      return;

    if (cptr->getSharedCounter() == 0) {
      cptr->killControlBlock();
    }
  }
};
