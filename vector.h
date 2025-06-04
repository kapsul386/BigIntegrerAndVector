#ifndef VECTOR_H
#define VECTOR_H

#define VECTOR_MEMORY_IMPLEMENTED

#include <cstddef>
#include <utility>
#include <stdexcept>
#include <iterator>
#include <initializer_list>
#include <type_traits>

class ArrayOutOfRange : public std::out_of_range {
 public:
  ArrayOutOfRange() : std::out_of_range("Array index out of range") {
  }
};

template <typename T>
class Vector {
 public:
  using ValueType = T;
  using Pointer = T*;
  using ConstPointer = const T*;
  using Reference = T&;
  using ConstReference = const T&;
  using SizeType = size_t;

  using Iterator = T*;
  using ConstIterator = const T*;
  using ReverseIterator = std::reverse_iterator<Iterator>;
  using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

  Vector() noexcept = default;

  explicit Vector(SizeType size) {
    if (size > 0) {
      data_ = static_cast<Pointer>(::operator new(size * sizeof(T)));
      capacity_ = size;

      SizeType i = 0;
      try {
        for (; i < size; ++i) {
          ::new (static_cast<void*>(data_ + i)) T();
        }
      } catch (...) {
        for (SizeType j = 0; j < i; ++j) {
          std::destroy_at(data_ + j);
        }
        ::operator delete(data_);
        data_ = nullptr;
        capacity_ = 0;
        throw;
      }
      size_ = size;
    }
  }

  Vector(SizeType size, const T& value) {
    if (size > 0) {
      data_ = static_cast<Pointer>(::operator new(size * sizeof(T)));
      capacity_ = size;

      SizeType i = 0;
      try {
        for (; i < size; ++i) {
          ::new (static_cast<void*>(data_ + i)) T(value);
        }
      } catch (...) {
        for (SizeType j = 0; j < i; ++j) {
          std::destroy_at(data_ + j);
        }
        ::operator delete(data_);
        data_ = nullptr;
        capacity_ = 0;
        throw;
      }
      size_ = size;
    }
  }

  Vector(std::initializer_list<T> list) : Vector(list.begin(), list.end()) {
  }

  template <class InputIt, class = std::enable_if_t<std::is_base_of_v<
                               std::input_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category> > >
  Vector(InputIt first, InputIt last) {
    SizeType count = 0;
    for (auto it = first; it != last; ++it) {
      ++count;
    }

    if (count > 0) {
      data_ = static_cast<Pointer>(::operator new(count * sizeof(T)));
      capacity_ = count;

      SizeType i = 0;
      try {
        for (auto it = first; it != last; ++it, ++i) {
          ::new (static_cast<void*>(data_ + i)) T(*it);
        }
      } catch (...) {
        for (SizeType j = 0; j < i; ++j) {
          std::destroy_at(data_ + j);
        }
        ::operator delete(data_);
        data_ = nullptr;
        capacity_ = 0;
        throw;
      }
      size_ = count;
    }
  }

  Vector(const Vector& other) {
    if (other.size_ > 0) {
      data_ = static_cast<Pointer>(::operator new(other.capacity_ * sizeof(T)));
      capacity_ = other.capacity_;

      SizeType i = 0;
      try {
        for (; i < other.size_; ++i) {
          ::new (static_cast<void*>(data_ + i)) T(other.data_[i]);
        }
      } catch (...) {
        for (SizeType j = 0; j < i; ++j) {
          std::destroy_at(data_ + j);
        }
        ::operator delete(data_);
        data_ = nullptr;
        capacity_ = 0;
        throw;
      }
      size_ = other.size_;
    }
  }

  Vector(Vector&& other) noexcept {
    data_ = other.data_;
    size_ = other.size_;
    capacity_ = other.capacity_;

    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
  }

  Vector& operator=(const Vector& other) {
    if (this != &other) {
      Vector tmp(other);
      Swap(tmp);
    }
    return *this;
  }

  Vector& operator=(Vector&& other) noexcept {
    if (this != &other) {
      Clear();
      Deallocate();
      data_ = other.data_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      other.data_ = nullptr;
      other.size_ = 0;
      other.capacity_ = 0;
    }
    return *this;
  }

  ~Vector() {
    Clear();
    Deallocate();
  }

  SizeType Size() const noexcept {
    return size_;
  }
  SizeType Capacity() const noexcept {
    return capacity_;
  }
  bool Empty() const noexcept {
    return size_ == 0;
  }

  Reference operator[](SizeType idx) {
    return data_[idx];
  }
  ConstReference operator[](SizeType idx) const {
    return data_[idx];
  }

  Reference At(SizeType idx) {
    if (idx >= size_) {
      throw ArrayOutOfRange();
    }
    return data_[idx];
  }
  ConstReference At(SizeType idx) const {
    if (idx >= size_) {
      throw ArrayOutOfRange();
    }
    return data_[idx];
  }

  Reference Front() {
    return data_[0];
  }
  ConstReference Front() const {
    return data_[0];
  }
  Reference Back() {
    return data_[size_ - 1];
  }
  ConstReference Back() const {
    return data_[size_ - 1];
  }

  Pointer Data() noexcept {
    return data_;
  }
  ConstPointer Data() const noexcept {
    return data_;
  }

  void Swap(Vector& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
  }

  void Resize(SizeType new_size) {
    if (new_size < size_) {
      for (SizeType i = new_size; i < size_; ++i) {
        std::destroy_at(data_ + i);
      }
      size_ = new_size;
      return;
    }

    if (new_size <= capacity_) {
      SizeType i = size_;
      try {
        for (; i < new_size; ++i) {
          ::new (static_cast<void*>(data_ + i)) T();
        }
      } catch (...) {
        for (SizeType j = size_; j < i; ++j) {
          std::destroy_at(data_ + j);
        }
        throw;
      }
      size_ = new_size;
      return;
    }

    SizeType desired = new_size;
    auto [new_data, new_cap] = AllocateMoreBuffer(desired);

    SizeType moved = 0;
    try {
      for (; moved < size_; ++moved) {
        ::new (static_cast<void*>(new_data + moved)) T(std::move_if_noexcept(data_[moved]));
      }
    } catch (...) {
      for (SizeType j = 0; j < moved; ++j) {
        std::destroy_at(new_data + j);
      }
      ::operator delete(new_data);
      throw;
    }

    SizeType constructed_tail = moved;
    try {
      for (; constructed_tail < new_size; ++constructed_tail) {
        ::new (static_cast<void*>(new_data + constructed_tail)) T();
      }
    } catch (...) {
      for (SizeType j = 0; j < constructed_tail; ++j) {
        std::destroy_at(new_data + j);
      }
      ::operator delete(new_data);
      throw;
    }

    for (SizeType i = 0; i < size_; ++i) {
      std::destroy_at(data_ + i);
    }
    ::operator delete(data_);

    data_ = new_data;
    capacity_ = new_cap;
    size_ = new_size;
  }

  void Resize(SizeType new_size, const T& value) {
    if (new_size < size_) {
      for (SizeType i = new_size; i < size_; ++i) {
        std::destroy_at(data_ + i);
      }
      size_ = new_size;
      return;
    }

    if (new_size <= capacity_) {
      SizeType i = size_;
      try {
        for (; i < new_size; ++i) {
          ::new (static_cast<void*>(data_ + i)) T(value);
        }
      } catch (...) {
        for (SizeType j = size_; j < i; ++j) {
          std::destroy_at(data_ + j);
        }
        throw;
      }
      size_ = new_size;
      return;
    }

    SizeType desired = new_size;
    auto [new_data, new_cap] = AllocateMoreBuffer(desired);

    SizeType moved = 0;
    try {
      for (; moved < size_; ++moved) {
        ::new (static_cast<void*>(new_data + moved)) T(std::move_if_noexcept(data_[moved]));
      }
    } catch (...) {
      for (SizeType j = 0; j < moved; ++j) {
        std::destroy_at(new_data + j);
      }
      ::operator delete(new_data);
      throw;
    }

    SizeType i = moved;
    try {
      for (; i < new_size; ++i) {
        ::new (static_cast<void*>(new_data + i)) T(value);
      }
    } catch (...) {
      for (SizeType j = 0; j < i; ++j) {
        std::destroy_at(new_data + j);
      }
      ::operator delete(new_data);
      throw;
    }

    for (SizeType j = 0; j < size_; ++j) {
      std::destroy_at(data_ + j);
    }
    ::operator delete(data_);

    data_ = new_data;
    capacity_ = new_cap;
    size_ = new_size;
  }

  void Reserve(SizeType new_cap) {
    if (new_cap <= capacity_) {
      return;
    }

    auto [new_data, confirmed_cap] = AllocateMoreBuffer(new_cap);

    SizeType moved = 0;
    try {
      for (; moved < size_; ++moved) {
        ::new (static_cast<void*>(new_data + moved)) T(std::move_if_noexcept(data_[moved]));
      }
    } catch (...) {
      for (SizeType j = 0; j < moved; ++j) {
        std::destroy_at(new_data + j);
      }
      ::operator delete(new_data);
      throw;
    }

    for (SizeType j = 0; j < size_; ++j) {
      std::destroy_at(data_ + j);
    }
    ::operator delete(data_);

    data_ = new_data;
    capacity_ = confirmed_cap;
  }

  void ShrinkToFit() {
    if (size_ == 0) {
      Deallocate();
      return;
    }
    if (capacity_ == size_) {
      return;
    }

    auto new_data = static_cast<Pointer>(::operator new(size_ * sizeof(T)));
    SizeType i = 0;
    try {
      for (; i < size_; ++i) {
        ::new (static_cast<void*>(new_data + i)) T(std::move_if_noexcept(data_[i]));
      }
    } catch (...) {
      for (SizeType j = 0; j < i; ++j) {
        std::destroy_at(new_data + j);
      }
      ::operator delete(new_data);
      throw;
    }

    for (SizeType j = 0; j < size_; ++j) {
      std::destroy_at(data_ + j);
    }
    ::operator delete(data_);

    data_ = new_data;
    capacity_ = size_;
  }

  void Clear() noexcept {
    for (SizeType i = 0; i < size_; ++i) {
      std::destroy_at(data_ + i);
    }
    size_ = 0;
  }

  void PushBack(const T& value) {
    if (size_ < capacity_) {
      ::new (static_cast<void*>(data_ + size_)) T(value);
      ++size_;
      return;
    }

    SizeType desired = size_ + 1;
    auto [new_data, new_cap] = AllocateMoreBuffer(desired);

    SizeType moved = 0;
    try {
      for (; moved < size_; ++moved) {
        ::new (static_cast<void*>(new_data + moved)) T(std::move_if_noexcept(data_[moved]));
      }
    } catch (...) {
      for (SizeType j = 0; j < moved; ++j) {
        std::destroy_at(new_data + j);
      }
      ::operator delete(new_data);
      throw;
    }

    SizeType idx_new = size_;
    try {
      ::new (static_cast<void*>(new_data + idx_new)) T(value);
    } catch (...) {
      for (SizeType j = 0; j < moved; ++j) {
        std::destroy_at(new_data + j);
      }
      ::operator delete(new_data);
      throw;
    }

    for (SizeType j = 0; j < size_; ++j) {
      std::destroy_at(data_ + j);
    }
    ::operator delete(data_);

    data_ = new_data;
    capacity_ = new_cap;
    size_ = moved + 1;
  }

  void PushBack(T&& value) {
    if (size_ < capacity_) {
      ::new (static_cast<void*>(data_ + size_)) T(std::move(value));
      ++size_;
      return;
    }

    SizeType desired = size_ + 1;
    auto [new_data, new_cap] = AllocateMoreBuffer(desired);

    SizeType moved = 0;
    try {
      for (; moved < size_; ++moved) {
        ::new (static_cast<void*>(new_data + moved)) T(std::move_if_noexcept(data_[moved]));
      }
    } catch (...) {
      for (SizeType j = 0; j < moved; ++j) {
        std::destroy_at(new_data + j);
      }
      ::operator delete(new_data);
      throw;
    }

    SizeType idx_new = size_;
    try {
      ::new (static_cast<void*>(new_data + idx_new)) T(std::move(value));
    } catch (...) {
      for (SizeType j = 0; j < moved; ++j) {
        std::destroy_at(new_data + j);
      }
      ::operator delete(new_data);
      throw;
    }

    for (SizeType j = 0; j < size_; ++j) {
      std::destroy_at(data_ + j);
    }
    ::operator delete(data_);

    data_ = new_data;
    capacity_ = new_cap;
    size_ = moved + 1;
  }

  template <typename... Args>
  void EmplaceBack(Args&&... args) {
    if (size_ < capacity_) {
      ::new (static_cast<void*>(data_ + size_)) T(std::forward<Args>(args)...);
      ++size_;
      return;
    }

    SizeType desired = size_ + 1;
    auto [new_data, new_cap] = AllocateMoreBuffer(desired);

    SizeType moved = 0;
    try {
      for (; moved < size_; ++moved) {
        ::new (static_cast<void*>(new_data + moved)) T(std::move_if_noexcept(data_[moved]));
      }
    } catch (...) {
      for (SizeType j = 0; j < moved; ++j) {
        std::destroy_at(new_data + j);
      }
      ::operator delete(new_data);
      throw;
    }

    SizeType idx_new = size_;
    try {
      ::new (static_cast<void*>(new_data + idx_new)) T(std::forward<Args>(args)...);
    } catch (...) {
      for (SizeType j = 0; j < moved; ++j) {
        std::destroy_at(new_data + j);
      }
      ::operator delete(new_data);
      throw;
    }

    for (SizeType j = 0; j < size_; ++j) {
      std::destroy_at(data_ + j);
    }
    ::operator delete(data_);

    data_ = new_data;
    capacity_ = new_cap;
    size_ = moved + 1;
  }

  void PopBack() {
    if (size_ > 0) {
      std::destroy_at(data_ + (size_ - 1));
      --size_;
    }
  }

  Iterator begin() noexcept {  // NOLINT
    return data_;
  }
  Iterator end() noexcept {  // NOLINT
    return data_ + size_;
  }
  ConstIterator begin() const noexcept {  // NOLINT
    return data_;
  }
  ConstIterator end() const noexcept {  // NOLINT
    return data_ + size_;
  }
  ConstIterator cbegin() const noexcept {  // NOLINT
    return data_;
  }
  ConstIterator cend() const noexcept {  // NOLINT
    return data_ + size_;
  }

  ReverseIterator rbegin() noexcept {  // NOLINT
    return ReverseIterator(end());
  }
  ReverseIterator rend() noexcept {  // NOLINT
    return ReverseIterator(begin());
  }
  ConstReverseIterator rbegin() const noexcept {  // NOLINT
    return ConstReverseIterator(end());
  }
  ConstReverseIterator rend() const noexcept {  // NOLINT
    return ConstReverseIterator(begin());
  }
  ConstReverseIterator crbegin() const noexcept {  // NOLINT
    return ConstReverseIterator(cend());
  }
  ConstReverseIterator crend() const noexcept {  // NOLINT
    return ConstReverseIterator(cbegin());
  }

  friend bool operator==(const Vector& lhs, const Vector& rhs) {
    if (lhs.size_ != rhs.size_) {
      return false;
    }
    for (SizeType i = 0; i < lhs.size_; ++i) {
      if (!(lhs.data_[i] == rhs.data_[i])) {
        return false;
      }
    }
    return true;
  }
  friend bool operator!=(const Vector& lhs, const Vector& rhs) {
    return !(lhs == rhs);
  }
  friend bool operator<(const Vector& lhs, const Vector& rhs) {
    SizeType n = (lhs.size_ < rhs.size_) ? lhs.size_ : rhs.size_;
    for (SizeType i = 0; i < n; ++i) {
      if (lhs.data_[i] < rhs.data_[i]) {
        return true;
      }
      if (rhs.data_[i] < lhs.data_[i]) {
        return false;
      }
    }
    return lhs.size_ < rhs.size_;
  }
  friend bool operator<=(const Vector& lhs, const Vector& rhs) {
    return !(rhs < lhs);
  }
  friend bool operator>(const Vector& lhs, const Vector& rhs) {
    return rhs < lhs;
  }
  friend bool operator>=(const Vector& lhs, const Vector& rhs) {
    return !(lhs < rhs);
  }

 private:
  Pointer data_ = nullptr;
  SizeType size_ = 0;
  SizeType capacity_ = 0;

  void Deallocate() noexcept {
    if (data_ != nullptr) {
      ::operator delete(data_);
      data_ = nullptr;
      capacity_ = 0;
    }
  }

  std::pair<Pointer, SizeType> AllocateMoreBuffer(SizeType min_cap) {
    SizeType new_cap = (capacity_ == 0 ? 1 : capacity_ * 2);
    if (new_cap < min_cap) {
      new_cap = min_cap;
    }
    auto new_data = static_cast<Pointer>(::operator new(new_cap * sizeof(T)));
    return {new_data, new_cap};
  }
};

#endif  // VECTOR_H
