#include <iostream>

namespace my {

template<typename T>
class shared_ptr {
 private:
  T* ptr;
  size_t* count;

 public:
  
  shared_ptr(T* ptr): ptr(ptr), count(new size_t(1)) {}

  shared_ptr(const shared_ptr& other): ptr(other.ptr), count(other.count) {
    ++*count;
  }

  shared_ptr(shared_ptr&& other) noexcept 
    : ptr(other.ptr), count(other.count) 
  {
    other.ptr = nullptr;
    other.count = nullptr;
  }

  ~shared_ptr() {
    if (!count) return;
    --*count;
    if (*count == 0) {
      delete ptr;
      delete count;
    }
  }

};


}
