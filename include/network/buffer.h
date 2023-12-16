#pragma once

#include <bitset>
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace network {

class buffer {
public:
  using ptr = std::shared_ptr<buffer>;

  buffer() = default;
  virtual ~buffer() = default;

  // return underlying data
  virtual char *data() const = 0;
  virtual size_t size() const = 0;
  virtual size_t get_capacity() const { return size(); }

  // convert to string
  virtual std::string to_string() const { return std::string(data(), size()); }
};

/**
  for std::string

  ************************ (underlying data for string)
      |----------------|
    offset    len
*/
class buffer_string : public buffer {
public:
  using ptr = std::shared_ptr<buffer_string>;
  buffer_string(std::string data, size_t offset = 0, size_t len = 0)
      : data_{std::move(data)} {
    setup(offset, len);
  }

  ~buffer_string() override = default;

  // override
  char *data() const override {
    return const_cast<char *>(data_.c_str()) + offset_;
  }
  size_t size() const override { return len_; }

private:
  void setup(size_t offset, size_t len) {
    auto str_size = data_.size();
    assert(str_size >= offset + len);
    // if len is not specified, then the size is the length of the remaining
    // data starting from offset
    if (!len) {
      len = str_size - offset;
    }
    offset_ = offset;
    len_ = len;
  }

private:
  std::string data_;
  size_t offset_;
  size_t len_;
};

class buffer_raw : public buffer {
public:
  using ptr = std::shared_ptr<buffer_raw>;

  static ptr create() { return ptr(new buffer_raw()); }

  buffer_raw(size_t capacity = 0) { set_capacity(capacity); }

  buffer_raw(const char *data, size_t size = 0) { assign(data, size); }

  ~buffer_raw() { clean(); }

  // copy constructor
  buffer_raw(const buffer_raw &other) : buffer_raw(other.get_capacity()) {
    // if data is valid, then copy
    if (other.data() && other.size()) {
      set_size(other.size());
      std::memcpy(data_, other.data(), size_);
    }
  }

  // move constructor
  buffer_raw(buffer_raw &&other) noexcept
      : size_{other.size_}, capacity_{other.capacity_}, data_{std::exchange(
                                                            other.data_,
                                                            nullptr)} {}
  // copy assignment
  buffer_raw &operator=(const buffer_raw &other) {
    return *this = buffer_raw(other);
  }

  // move assignment
  buffer_raw &operator=(buffer_raw &&other) noexcept {
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
    std::swap(data_, other.data_);
    return *this;
  }

  // override
  char *data() const override { return data_; }
  size_t size() const override { return size_; }
  size_t get_capacity() const { return capacity_; }

  // set memory capacity
  void set_capacity(size_t capacity) {
    // if data is empty, then allocate directly
    if (!data_ && capacity) {
      set_size(0);
      goto alloc;
    }

    if (data_ && !capacity) {
      clean();
      return;
    }

    // if newly requested capacity is bigger than capacity_, delete original
    if (capacity > capacity_) {
      delete[] data_;
      data_ = nullptr;
      set_size(0);
      goto alloc;

      // if lower than 2k or bigger than half of original space then reuse the
      // old memory
    } else if (capacity < 2 * 1024 || 2 * capacity > capacity_) {
      return;
    }

  alloc:
    data_ = new char[capacity];
    capacity_ = capacity;
  }

  // set actual size
  void set_size(size_t size) {
    if (size > capacity_) {
      throw std::invalid_argument(
          "buffer cannot set size bigger than capacity");
    }

    size_ = size;
  }

  void assign(const char *data, size_t size = 0) {
    if (!data || !size) {
      clean();
      return;
    }

    set_capacity(size);
    std::memcpy(data_, data, size);
    set_size(size);
  }

  void clean() {
    if (data_) {
      delete[] data_;
      data_ = nullptr;
    }

    size_ = 0;
    capacity_ = 0;
  }

  void dump() {
    if (!data_) {
      std::cout << "null data" << std::endl;
      return;
    }

    for (auto i = 0; i < std::strlen(data_) + 1; ++i) {
      std::cout << data_[i];
    }
    std::cout << std::endl;

    for (auto i = 0; i < std::strlen(data_) + 1; ++i) {
      std::bitset<8> b(data_[i]);
      std::cout << b;
      std::cout << " ";
    }
    std::cout << std::endl;
  }

private:
  size_t size_{0};
  size_t capacity_{0};
  char *data_{nullptr};
};

} // namespace network
