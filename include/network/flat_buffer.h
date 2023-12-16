#pragma once

#include <bitset>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <utility>

#include <arpa/inet.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>

namespace network {

/*
   Memory is laid out thusly:

data_[0] --- read_index_ --- write_index_ ---
capacity_
*/

class flat_buffer {
public:
  using ptr = std::shared_ptr<flat_buffer>;

  static constexpr size_t kInitialSize = 1024;

  static ptr create(size_t initial_size = kInitialSize) {
    return std::make_shared<flat_buffer>(initial_size);
  }

  explicit flat_buffer(size_t initial_size = kInitialSize)
      : capacity_{initial_size} {
    read_index_ = write_index_ = 0;
    data_ = new char[capacity_];
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
        write_index_{other.write_index_} {
    data_ = new char[capacity_];
    std::memcpy(data_, other.data_, capacity_);
  }

  flat_buffer &operator=(const flat_buffer &other) {
    if (this != &other) {
      capacity_ = other.capacity_;
      read_index_ = other.read_index_;
      write_index_ = other.write_index_;
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
    }

    return *this;
  }

  void info() {
    spdlog::info("read_index = {}, write_index = {}, capacity = {}",
                 read_index_, write_index_, capacity_);
  }

  const char *data() const { return data_ + read_index_; }
  char *data() { return data_ + read_index_; }

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

  char *write_begin() { return begin() + write_index_; }
  const char *write_begin() const { return begin() + write_index_; }

  void write(const char *data, size_t size) {
    ensure_writable_bytes(size);
    std::memcpy(write_begin(), data, size);
    write_index_ += size;
  }

  void consume(size_t len) {
    if (len < unread_length()) {
      read_index_ += len;
    } else {
      throw std::runtime_error(
          fmt::format("flat_buffer cannot consume {} bytes, unread length = {}",
                      len, unread_length()));
    }
  }

  void unread_bytes(size_t len) {
    if (len > read_index_) {
      read_index_ = 0;
    } else {
      read_index_ -= len;
    }
  }

  // discard all the unread data
  void reset() { read_index_ = write_index_ = 0; }

  uint8_t peek_uint8() const {
    if (unread_length() < sizeof(uint8_t)) {
      throw std::runtime_error("not enough data");
    }

    uint8_t x = *data();
    return x;
  }

  uint8_t read_uint8() {
    uint8_t x = peek_uint8();
    consume(sizeof(x));
    return x;
  }

  uint16_t peek_uint16() const {
    if (unread_length() < sizeof(uint16_t)) {
      throw std::runtime_error("not enough data");
    }

    uint16_t x = 0;
    std::memcpy(&x, data(), sizeof x);
    return ntohs(x);
  }

  uint16_t read_uint16() {
    uint16_t x = peek_uint16();
    consume(sizeof(x));
    return x;
  }

  uint32_t peek_uint32() const {
    if (unread_length() < sizeof(uint32_t)) {
      throw std::runtime_error("not enough data");
    }

    uint32_t x = 0;
    std::memcpy(&x, data(), sizeof(x));
    return ntohl(x);
  }

  uint32_t read_uint32() {
    uint32_t x = peek_uint32();
    consume(sizeof(x));
    return x;
  }

  uint64_t peek_uint64() {
    if (unread_length() < sizeof(uint64_t)) {
      throw std::runtime_error("not enough data");
    }

    uint32_t first = peek_uint32();
    consume(sizeof(uint32_t));
    uint32_t second = peek_uint32();
    unread_bytes(sizeof(uint32_t));

    return ((uint64_t)first << 32) | second;
  }

  uint64_t read_uint64() {
    uint64_t x = peek_uint64();
    consume(sizeof(x));
    return x;
  }

  std::string to_string(size_t len) {
    if (len > unread_length()) {
      throw std::runtime_error("not enough data");
    }

    std::string s(data(), len);
    consume(len);
    return s;
  }

  void dump() {
    if (!data()) {
      std::cout << "null data" << std::endl;
      return;
    }

    std::stringstream ss;
    ss << std::hex << std::uppercase;
    for (auto i = 0; i < unread_length(); ++i) {
      char c = data()[i];
      std::bitset<8> b(c);
      if (((uint8_t)c) < 16) {
        ss << "0";
      }

      std::cout << b;
      std::cout << " ";
      ss << b.to_ulong() << " ";
      if ((i + 1) % 8 == 0) {
        std::cout << "\n";
        ss << "\n";
      }
    }

    std::cout << std::endl;

    std::cout << ss.str() << std::endl;
  }

private:
  char *begin() { return data_; }
  const char *begin() const { return data_; }

  void ensure_writable_bytes(size_t n) {
    if (writable_bytes() < n) {
      grow(n);
    }
  }

  // grow the buffer to hold len bytes in written area
  void grow(size_t len) {
    auto available_bytes = writable_bytes();
    if (available_bytes < len) {
      // grow the capacity
      size_t capacity = (capacity_ << 1) + len;
      size_t unconsumed_data_length = unread_length();
      char *temp = new char[capacity];
      // copy the data that hasn't been consumed
      std::memcpy(temp, begin() + read_index_, unconsumed_data_length);
      read_index_ = 0;
      write_index_ = unconsumed_data_length;
      capacity_ = capacity;
      if (data_) {
        delete[] data_;
        data_ = nullptr;
      }
      data_ = temp;
    } else {
      // move readable data to the front, make space inside buffer
      size_t unconsumed_data_length = unread_length();
      std::memmove(begin(), begin() + read_index_, unconsumed_data_length);
      read_index_ = 0;
      write_index_ = unconsumed_data_length;
    }
  }

private:
  char *data_{nullptr};
  size_t capacity_;
  size_t read_index_;
  size_t write_index_;
};

} // namespace network