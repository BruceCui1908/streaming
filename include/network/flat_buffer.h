#pragma once

#include <cassert>
#include <cstring>
#include <memory>
#include <utility>

namespace network {

/*
   Memory is laid out thusly:

data_[0] --- reserved_prepend_size_ --- read_index_ --- write_index_ ---
capacity_
*/

class flat_buffer {
public:
  using ptr = std::shared_ptr<flat_buffer>;

  static constexpr size_t kCheapPrependSize = 8;
  static constexpr size_t kInitialSize = 1024;

  explicit flat_buffer(size_t initial_size = kInitialSize,
                       size_t reserved_prepend_size = kCheapPrependSize)
      : capacity_{initial_size + reserved_prepend_size},
        read_index_{reserved_prepend_size}, write_index_{reserved_prepend_size},
        reserved_prepend_size_{reserved_prepend_size} {
    data_ = new char[capacity_];
    assert(unread_length() == 0);
    assert(writable_bytes() == initial_size);
    assert(prependable_bytes() == reserved_prepend_size);
  }

  ~flat_buffer() {
    if (data_) {
      delete[] data_;
      data_ = nullptr;
    }
    capacity_ = 0;
  }

  flat_buffer(const flat_buffer &other)
      : capacity_{other.capacity_}, read_index_{other.read_index_},
        write_index_{other.write_index_}, reserved_prepend_size_{
                                              other.reserved_prepend_size_} {
    data_ = new char[capacity_];
    std::memcpy(data_, other.data_, capacity_);
  }

  flat_buffer &operator=(const flat_buffer &other) {
    if (this != &other) {
      capacity_ = other.capacity_;
      read_index_ = other.read_index_;
      write_index_ = other.write_index_;
      reserved_prepend_size_ = other.reserved_prepend_size_;
      if (data_) {
        delete[] data_;
        data_ = nullptr;
      }
      data_ = new char[capacity_];
      std::memcpy(data_, other.data_, capacity_);
    }
    return *this;
  }

  flat_buffer(flat_buffer &&other) noexcept
      : data_(std::exchange(other.data_, nullptr)) {
    std::swap(capacity_, other.capacity_);
    std::swap(read_index_, other.read_index_);
    std::swap(write_index_, other.write_index_);
    std::swap(reserved_prepend_size_, other.reserved_prepend_size_);
  }

  flat_buffer &operator=(flat_buffer &&other) noexcept {
    if (this != &other) {
      if (data_) {
        delete[] data_;
        data_ = nullptr;
      }
      std::swap(data_, other.data_);
      std::swap(capacity_, other.capacity_);
      std::swap(read_index_, other.read_index_);
      std::swap(write_index_, other.write_index_);
      std::swap(reserved_prepend_size_, other.reserved_prepend_size_);
    }

    return *this;
  }

  const char *data() const { return data_ + read_index_; }

  // length returns the number of bytes of the unread portion of the buffer
  size_t unread_length() const {
    assert(write_index_ >= read_index_);
    return write_index_ - read_index_;
  }

  size_t capacity() const { return capacity_; }

  size_t writable_bytes() const {
    assert(capacity_ >= write_index_);
    return capacity_ - write_index_;
  }

  size_t prependable_bytes() const { return read_index_; }

  char *write_begin() { return begin() + write_index_; }
  const char *write_begin() const { return begin() + write_index_; }

  void reserve(size_t len) {
    if (capacity_ >= len + reserved_prepend_size_) {
      return;
    }

    grow(len);
  }

  void ensure_writable_bytes(size_t n) {
    if (writable_bytes() < n) {
      grow(n);
    }
  }

  void write(const char *data, size_t size) {
    ensure_writable_bytes(size);
    std::memcpy(write_begin(), data, size);
    write_index_ += size;
  }

  void prepend(const char *data, size_t size) {
    assert(size <= prependable_bytes());
    read_index_ -= size;
    std::memcpy(begin() + read_index_, data, size);
  }

  void consume(size_t len) {
    if (len < unread_length()) {
      read_index_ += len;
    } else {
      reset();
    }
  }

  // Truncate discards all but the first n unread bytes from the buffer
  // but continues to use the same allocated storage.
  // It does nothing if n is greater than the length of the buffer.
  void truncate(size_t size) {
    if (size == 0) {
      write_index_ = read_index_ = reserved_prepend_size_;
    } else if (write_index_ > read_index_ + size) {
      write_index_ = read_index_ + size;
    }
  }

  // discard all the unread data
  void reset() { truncate(0); }

private:
  char *begin() { return data_; }
  const char *begin() const { return data_; }

  // grow the buffer to hold len bytes in written area
  void grow(size_t len) {
    auto available_bytes = writable_bytes() + prependable_bytes();
    if (available_bytes < len + reserved_prepend_size_) {
      // grow the capacity
      size_t capacity = (capacity_ << 1) + len;
      size_t unconsumed_data_length = unread_length();
      char *temp = new char[capacity];
      // copy the data that hasn't been consumed
      std::memcpy(temp + reserved_prepend_size_, begin() + read_index_,
                  unconsumed_data_length);
      read_index_ = reserved_prepend_size_;
      write_index_ = reserved_prepend_size_ + unconsumed_data_length;
      capacity_ = capacity;
      if (data_) {
        delete[] data_;
        data_ = nullptr;
      }
      data_ = temp;
    } else {
      // move readable data to the front, make space inside buffer
      size_t unconsumed_data_length = unread_length();
      std::memmove(begin() + reserved_prepend_size_, begin() + read_index_,
                   unconsumed_data_length);
      read_index_ = reserved_prepend_size_;
      write_index_ = read_index_ + unconsumed_data_length;
    }
  }

private:
  char *data_;
  size_t capacity_;
  size_t read_index_;
  size_t write_index_;
  size_t reserved_prepend_size_;
};

} // namespace network