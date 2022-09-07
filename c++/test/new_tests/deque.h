#pragma once
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <utility>

template<typename T>
class Deque {
 public:
// 0 .. num_backets - 1
// 0 .. kBacketSize - 1

  void push_back(const T&);
  void push_front(const T&);
  void pop_front();
  void pop_back();

  T& operator[](size_t pos);
  const T& operator[](size_t pos) const;
  Deque<T>& operator=(const Deque<T>&);

  Deque();
  Deque(const Deque<T>&);
  Deque(size_t, const T&);
  Deque(size_t);
  ~Deque();

  T& at(size_t pos);
  const T& at(size_t pos) const;

  size_t size() const;

  template<bool is_const>
  struct base_iterator {
   public:
 
    using reference = std::conditional_t<is_const, const T&, T&>;
    using pointer = std::conditional_t<is_const, const T*, T*>;
    using value_type = T;
    using difference_type = long long;
    using iterator_category = std::random_access_iterator_tag;

    bool operator!=(base_iterator other) const;
    bool operator>(base_iterator other) const;
    bool operator<(base_iterator other) const;
    bool operator>=(base_iterator other) const;
    bool operator<=(base_iterator other) const;
    bool operator==(base_iterator other) const;
    difference_type operator-(base_iterator other) const;
    base_iterator operator-(long long) const;
    base_iterator operator+(long long) const;
    base_iterator(T** backet, size_t position);
    base_iterator(const base_iterator<false>& other)
      : backet_(other.backet_)
      , position_in_backet_(other.position_in_backet_)
    {}
    base_iterator operator++(int);
    base_iterator operator--(int);
    base_iterator& operator++();
    base_iterator& operator--();
    base_iterator& operator+=(size_t);
    base_iterator& operator-=(size_t);
    base_iterator& operator=(const base_iterator&) = default;
    std::conditional_t<is_const, const T&, T&> operator*();
    std::conditional_t<is_const, const T*, T*> operator->();
    ~base_iterator() = default;

   private:
    
    T** backet_;
    size_t position_in_backet_;
  };

  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator cbegin() const;
  const_iterator end() const;
  const_iterator cend() const;
  reverse_iterator rend() { return std::make_reverse_iterator(begin()); }
  reverse_iterator rbegin() { return std::make_reverse_iterator(end()); }
  const_reverse_iterator crend() const { return std::make_reverse_iterator(cbegin()); }
  const_reverse_iterator crbegin() const { return std::make_reverse_iterator(cend()); }
  const_reverse_iterator rend() const { return std::make_reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const { return std::make_reverse_iterator(end()); }

  void insert(iterator, const T&);
  void erase(iterator);

 private:

  static const size_t kBacketSize = 100;
  T** data_;

  size_t number_backets_;
  size_t first_used_backet_;
  size_t first_used_index_;
  size_t last_used_backet_;
  size_t last_non_used_index_;
  size_t size_;

  void Swap(Deque<T>& , Deque<T>&);
  void FreeMemory();
  void GetNewCapacity(size_t new_size, T**& new_data, 
                      size_t& new_number_backets) const;
  void ResizeAndMove(size_t new_size);
  void MoveValues(T**& new_data, size_t new_number_backets);
};

template<typename T>
Deque<T>::~Deque() {
  FreeMemory();
}

template<typename T>
template<bool is_const>
std::conditional_t<is_const, const T&, T&>
Deque<T>::base_iterator<is_const>::operator*() {
  return (*backet_)[position_in_backet_];
}

template<typename T>
template<bool is_const>
std::conditional_t<is_const, const T*, T*>
Deque<T>::base_iterator<is_const>::operator->() {
  return &(*backet_)[position_in_backet_];
}

template<typename T>
void Deque<T>::FreeMemory() {
  while (size_ != 0) {
    pop_back();
  }
  for (size_t i = 0; i < number_backets_; ++i) {
    delete [] reinterpret_cast<uint8_t*>(data_[i]);
  }
  delete [] data_;
  data_ = nullptr;
}

template<typename T>
void Deque<T>::GetNewCapacity(size_t count, T**& new_data,
                              size_t& new_number_backets) const {
  new_number_backets = (count + kBacketSize - 1) / kBacketSize;
  new_data = new T* [new_number_backets];
}

template<typename T>
void Deque<T>::ResizeAndMove(size_t element_count) {
  T** new_data;
  size_t new_number_backets;
    GetNewCapacity(element_count, new_data, new_number_backets);
    MoveValues(new_data, new_number_backets);
} 

template<typename T>
void Deque<T>::MoveValues(T**& new_data, size_t new_number_backets) {
  size_t border_first = (new_number_backets - number_backets_) / 2;
  size_t border_second = (new_number_backets - number_backets_) - border_first;
  size_t first_alloc_place = 0;
  size_t second_alloc_place = new_number_backets - border_second;
  try {
    if (data_ != nullptr) {
      for (first_alloc_place = 0; first_alloc_place < border_first; ++first_alloc_place) {
        new_data[first_alloc_place] = 
            reinterpret_cast<T*>(new uint8_t [kBacketSize * sizeof(T)]);
      }
      for (; second_alloc_place < new_number_backets; ++second_alloc_place) {
        new_data[second_alloc_place] = 
            reinterpret_cast<T*>(new uint8_t [kBacketSize * sizeof(T)]);
      }
      for (size_t i = 0; i < number_backets_; ++i) {
        new_data[i + border_first] = data_[i];
      }
    } else {
      new_data[first_alloc_place++] = 
            reinterpret_cast<T*>(new uint8_t [kBacketSize * sizeof(T)]);
    }
  } catch (...) {
    while (first_alloc_place) {
      delete [] new_data[--first_alloc_place];
    }
    while (second_alloc_place > new_number_backets - border_second) {
      delete [] new_data[--second_alloc_place];
    }
    delete [] new_data;
    throw;
  }
  number_backets_ = new_number_backets;
  delete [] data_;
  data_ = new_data;
  first_used_backet_ += border_first;
  last_used_backet_ += border_first;
}

template<typename T>
Deque<T>::Deque():
  data_(nullptr),
  number_backets_(0),
  first_used_backet_(0),
  first_used_index_(kBacketSize / 2),
  last_used_backet_(0),
  last_non_used_index_(kBacketSize / 2),
  size_(0)
{
    ResizeAndMove(kBacketSize);
}

// copypast 
template<typename T>
T& Deque<T>::operator[](size_t pos) {
  return data_[first_used_backet_ + (pos + first_used_index_) / kBacketSize]
              [(pos + first_used_index_) % kBacketSize];
}

template<typename T>
const T& Deque<T>::operator[](size_t pos) const {
  return data_[first_used_backet_ + (pos + first_used_index_) / kBacketSize]
              [(pos + first_used_index_) % kBacketSize];
}

template<typename T>
void Deque<T>::push_back(const T& value) {
    if (last_non_used_index_ == kBacketSize) {
      if (last_used_backet_ == number_backets_ - 1) {
        ResizeAndMove(number_backets_ * 3 * kBacketSize);
      }
      ++last_used_backet_;
      last_non_used_index_ = 0;
    }
    //std::cout << "this";
    new(&data_[last_used_backet_][last_non_used_index_]) T(value);
    ++last_non_used_index_;
    ++size_;
}

template<typename T>
void Deque<T>::push_front(const T& value) {
    if (first_used_index_ == 0) {
      if (first_used_backet_ == 0) {
        ResizeAndMove(number_backets_ * 3 * kBacketSize);
      }
      --first_used_backet_;
      first_used_index_ = kBacketSize;
    }
    new(&data_[first_used_backet_][first_used_index_ - 1]) T(value);
    --first_used_index_;
    ++size_;
}

template<typename T>
void Deque<T>::pop_back() {
  if (size_ == 0) {
    return;
  }
  if (last_non_used_index_ == 0) {
    --last_used_backet_;
    last_non_used_index_ = kBacketSize;
  }
  data_[last_used_backet_][--last_non_used_index_].~T();
  --size_;
}

template<typename T>
void Deque<T>::pop_front() {
  if (size_ == 0) {
    return;
  }
  if (first_used_index_ == kBacketSize) {
    ++first_used_backet_;
    first_used_index_ = 0;
  }
  data_[first_used_backet_][first_used_index_++].~T();
  --size_;
  if (first_used_index_ == kBacketSize) { ++first_used_backet_; first_used_index_ = 0; }
}

template<typename T>
Deque<T>::Deque(const Deque<T>& other):
  Deque()  
{
  ResizeAndMove(other.number_backets_ * kBacketSize);
  first_used_index_ = last_non_used_index_ = other.first_used_index_;
  first_used_backet_ = last_used_backet_ = other.first_used_backet_;
  for (size_t i = 0; i < other.size(); ++i) {
    push_back(other[i]);
  }
}

template<typename T>
void Deque<T>::Swap(Deque<T>& first, Deque<T>& second) {
  std::swap(first.data_, second.data_);
  std::swap(first.number_backets_, second.number_backets_);
  std::swap(first.first_used_backet_, second.first_used_backet_);
  std::swap(first.first_used_index_, second.first_used_index_);
  std::swap(first.last_used_backet_, second.last_used_backet_);
  std::swap(first.last_non_used_index_, second.last_non_used_index_);
  std::swap(first.size_, second.size_);
}

template<typename T>
Deque<T>& Deque<T>::operator=(const Deque<T>& other) {
  Deque<T> temp = other;
  Swap(*this, temp);
  return *this;
}

template<typename T>
size_t Deque<T>::size() const {
  return size_;
}

template<typename T>
Deque<T>::Deque(size_t count, const T& value):
  Deque()
{
  try {
    for (size_t i = 0; i < count; ++i) {
      push_back(value);
    }
  } catch (...) {
    while (size_) {
      pop_back();
    }
    //FreeMemory();
    throw;
  }
}

template<typename T>
Deque<T>::Deque(size_t count):
  Deque(count, T())
{}

template<typename T>
const T& Deque<T>::at(size_t pos) const {
  if (pos >= size()) {
    throw std::out_of_range("out_of_range");
  }
  return (*this)[pos];
}

template<typename T>
T& Deque<T>::at(size_t pos) {
  if (pos >= size()) {
    throw std::out_of_range("out_of_range");
  }
  return (*this)[pos];
}


template<typename T>
template<bool is_const>
Deque<T>::base_iterator<is_const>::base_iterator(T** backet, size_t position)
  : backet_(backet)
  , position_in_backet_(position)
{}

template<typename T>
template<bool is_const>
typename Deque<T>::template base_iterator<is_const>&
Deque<T>::base_iterator<is_const>::operator++() {
  backet_ += (++position_in_backet_) / kBacketSize;
  position_in_backet_ %= kBacketSize;
  return *this;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template base_iterator<is_const>
Deque<T>::base_iterator<is_const>::operator++(int) {
  typename Deque<T>::template base_iterator<is_const> temp = *this;
  backet_ += (++position_in_backet_) / kBacketSize;
  position_in_backet_ %= kBacketSize;
  return temp;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template base_iterator<is_const>&
Deque<T>::base_iterator<is_const>::operator--() {
  if (position_in_backet_ == 0) {
    position_in_backet_ = kBacketSize;
    --backet_;    
  }
  --position_in_backet_;
  return *this;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template base_iterator<is_const>
Deque<T>::base_iterator<is_const>::operator--(int) {
  typename Deque<T>::template base_iterator<is_const> temp = *this;
  --*this;
  return temp;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template base_iterator<is_const>
Deque<T>::base_iterator<is_const>::operator+(long long value) const {
  Deque<T>::base_iterator<is_const> temp(*this);
  if (value > 0) {
    temp += value;
  } else {
    temp -= -value;
  }
  return temp;
}

template<typename T>
template<bool is_const>
long long Deque<T>::template base_iterator<is_const>::operator-(
    typename Deque<T>::template base_iterator<is_const> other) const {
  size_t result = position_in_backet_ +
      (backet_ - other.backet_) * kBacketSize - other.position_in_backet_;
  return result;
}

template<typename T>
template<bool is_const>
bool Deque<T>::template base_iterator<is_const>::operator==(
    typename Deque<T>::template base_iterator<is_const> other) const {
  return backet_ == other.backet_ && 
         position_in_backet_ == other.position_in_backet_;
}

template<typename T>
template<bool is_const>
bool Deque<T>::base_iterator<is_const>::operator!=(
    typename Deque<T>::template base_iterator<is_const> other) const { 
  return !(*this == other); 
}

template<typename T>
template<bool is_const>
bool Deque<T>::base_iterator<is_const>::operator>(
    typename Deque<T>::template base_iterator<is_const> other) const { 

  if (backet_ < other.backet_) { return false; }
  if (backet_ == other.backet_ && 
      position_in_backet_ <= other.position_in_backet_) { return false; }
  return true;
}

template<typename T>
template<bool is_const>
bool Deque<T>::base_iterator<is_const>::operator<(
    typename Deque<T>::template base_iterator<is_const> other) const { 
  return other > *this; 
}

template<typename T>
template<bool is_const>
bool Deque<T>::base_iterator<is_const>::operator>=(
    typename Deque<T>::template base_iterator<is_const> other) const { 
  return !(*this < other); 
}

template<typename T>
template<bool is_const>
bool Deque<T>::base_iterator<is_const>::operator<=(
    typename Deque<T>::template base_iterator<is_const> other) const { 
  return !(*this > other); 
}

template<typename T>
template<bool is_const>
typename Deque<T>::template base_iterator<is_const>& 
Deque<T>::template base_iterator<is_const>::operator+=(size_t value) {
  backet_ += (position_in_backet_ + value) / Deque<T>::kBacketSize;
  position_in_backet_ = (position_in_backet_ + value) % Deque<T>::kBacketSize; 
  return *this;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template base_iterator<is_const>& 
Deque<T>::base_iterator<is_const>::operator-=(size_t value) {
  if (position_in_backet_ >= value) {
    position_in_backet_ -= value;
    return *this;
  }
  backet_ -= 
      (value - position_in_backet_ + kBacketSize - 1) / kBacketSize;
  position_in_backet_ = 
      (kBacketSize - (value - position_in_backet_) % kBacketSize) % kBacketSize; 
  return *this;
}

template<typename T>
template<bool is_const>
typename Deque<T>::template base_iterator<is_const>
Deque<T>::base_iterator<is_const>::operator-(long long value) const {
  typename Deque<T>::template base_iterator<is_const> temp = *this;
  if (value > 0) {
    temp -= value;
  } else {
    temp += -value;
  }
  return temp;
}

template<typename T>
typename Deque<T>::iterator
Deque<T>::begin() {
  return Deque<T>::iterator(data_ + first_used_backet_, first_used_index_);
}

template<typename T>
typename Deque<T>::iterator
Deque<T>::end() {
  size_t current_index = last_non_used_index_;
  T** current_backet = data_ + last_used_backet_;
  if (last_non_used_index_ == kBacketSize) {
    current_index = 0;
    ++current_backet;
  }
  return Deque<T>::iterator(current_backet, current_index);
}

template<typename T>
typename Deque<T>::const_iterator
Deque<T>::begin() const {
  return const_iterator(data_ + first_used_backet_, first_used_index_);
}

template<typename T>
typename Deque<T>::const_iterator
Deque<T>::cbegin() const {
  return const_iterator(data_ + first_used_backet_, first_used_index_);
}

template<typename T>
typename Deque<T>::const_iterator
Deque<T>::end() const {
  return const_iterator(data_ + last_used_backet_, last_non_used_index_);
}

template<typename T>
typename Deque<T>::const_iterator
Deque<T>::cend() const {
  return const_iterator(data_ + last_used_backet_, last_non_used_index_);
}

template<typename T>
void Deque<T>::insert(iterator it, const T& value) {
  auto copy_this = *this;
  try {
    T copy = value;
    auto it_copy = it;
    for (; it_copy != end(); ++it_copy) {
      std::swap(copy, *it_copy);
    }
    push_back(copy);
  } catch (...) {
    Swap(*this, copy_this);
    throw;
  }
}

template<typename T>
void Deque<T>::erase(iterator it) {
  for (; it != end() - 1; ++it) {
    std::swap(*it, *(it + 1));
  }
  pop_back();
}
