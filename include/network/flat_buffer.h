#pragma once

#include <arpa/inet.h>

#include <spdlog/spdlog.h>

#include <bitset>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace network {

class flat_buffer
{
public:
    using ptr = std::shared_ptr<flat_buffer>;

    static constexpr size_t kInitialSize = 1024;

    static ptr create(size_t initial_size = kInitialSize)
    {
        return std::make_shared<flat_buffer>(initial_size);
    }

    explicit flat_buffer(size_t initial_size = kInitialSize)
        : capacity_{initial_size}
    {
        read_index_ = write_index_ = 0;
        data_ = new char[capacity_];
    }

    ~flat_buffer()
    {
        if (data_)
        {
            delete[] data_;
            data_ = nullptr;
        }
        capacity_ = 0;
    }

    flat_buffer(const flat_buffer &other)
        : capacity_{other.capacity_}
        , read_index_{other.read_index_}
        , write_index_{other.write_index_}
    {
        data_ = new char[capacity_];
        std::memcpy(data_, other.data_, capacity_);
    }

    flat_buffer &operator=(const flat_buffer &other)
    {
        if (this != &other)
        {
            capacity_ = other.capacity_;
            read_index_ = other.read_index_;
            write_index_ = other.write_index_;
            if (data_)
            {
                delete[] data_;
                data_ = nullptr;
            }
            data_ = new char[capacity_];
            std::memcpy(data_, other.data_, capacity_);
        }

        return *this;
    }

    flat_buffer(flat_buffer &&other) noexcept
        : data_(std::exchange(other.data_, nullptr))
    {
        std::swap(capacity_, other.capacity_);
        std::swap(read_index_, other.read_index_);
        std::swap(write_index_, other.write_index_);
    }

    flat_buffer &operator=(flat_buffer &&other) noexcept
    {
        if (this != &other)
        {
            if (data_)
            {
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

    void print()
    {
        spdlog::info("read_index = {}, write_index = {}, capacity = {}", read_index_, write_index_, capacity_);
    }

    const char *data() const
    {
        return data_ + read_index_;
    }
    char *data()
    {
        return data_ + read_index_;
    }
    char *write_begin()
    {
        return begin() + write_index_;
    }
    const char *write_begin() const
    {
        return begin() + write_index_;
    }

    // length returns the number of bytes of the unread portion of the buffer
    size_t unread_length() const
    {
        assert(write_index_ >= read_index_);
        return write_index_ - read_index_;
    }

    size_t capacity() const
    {
        return capacity_;
    }

    size_t writable_bytes() const
    {
        assert(capacity_ >= write_index_);
        return capacity_ - write_index_;
    }

    void require_length_or_fail(size_t len) const
    {
        if (len > unread_length())
        {
            throw std::runtime_error(fmt::format("flat_buffer {} does not contain the required length {}", unread_length(), len));
        }
    }

    void write(const char *data, size_t size)
    {
        ensure_writable_bytes(size);
        std::memcpy(write_begin(), data, size);
        write_index_ += size;
    }

    void consume_or_fail(size_t len)
    {
        if (len <= unread_length())
        {
            read_index_ += len;
        }
        else
        {
            throw std::runtime_error(fmt::format("flat_buffer cannot consume {} bytes, unread length = {}", len, unread_length()));
        }
    }

    void safe_consume(size_t len)
    {
        if (len <= unread_length())
        {
            read_index_ += len;
        }
        else
        {
            reset();
        }
    }

    void unread_bytes(size_t len)
    {
        if (len > read_index_)
        {
            read_index_ = 0;
        }
        else
        {
            read_index_ -= len;
        }
    }

    // discard all the unread data
    void reset()
    {
        read_index_ = write_index_ = 0;
    }

    uint8_t peek_uint8() const
    {
        if (unread_length() < sizeof(uint8_t))
        {
            throw std::runtime_error("not enough data");
        }

        uint8_t x = *data();
        return x;
    }

    uint8_t read_uint8()
    {
        uint8_t x = peek_uint8();
        consume_or_fail(sizeof(x));
        return x;
    }

    uint16_t peek_uint16() const
    {
        if (unread_length() < sizeof(uint16_t))
        {
            throw std::runtime_error("not enough data");
        }

        uint16_t x = 0;
        std::memcpy(&x, data(), sizeof x);
        return ntohs(x);
    }

    uint16_t read_uint16()
    {
        uint16_t x = peek_uint16();
        consume_or_fail(sizeof(x));
        return x;
    }

    uint32_t peek_uint32() const
    {
        if (unread_length() < sizeof(uint32_t))
        {
            throw std::runtime_error("not enough data");
        }

        uint32_t x = 0;
        std::memcpy(&x, data(), sizeof(x));
        return ntohl(x);
    }

    uint32_t read_uint32()
    {
        uint32_t x = peek_uint32();
        consume_or_fail(sizeof(x));
        return x;
    }

    uint64_t peek_uint64()
    {
        if (unread_length() < sizeof(uint64_t))
        {
            throw std::runtime_error("not enough data");
        }

        uint32_t first = peek_uint32();
        consume_or_fail(sizeof(uint32_t));
        uint32_t second = peek_uint32();
        unread_bytes(sizeof(uint32_t));

        return ((uint64_t)first << 32) | second;
    }

    uint64_t read_uint64()
    {
        uint64_t x = peek_uint64();
        consume_or_fail(sizeof(x));
        return x;
    }

    std::string to_string(size_t len)
    {
        if (len > unread_length())
        {
            throw std::runtime_error("not enough data");
        }

        std::string s(data(), len);
        consume_or_fail(len);
        return s;
    }

    void capture_snapshot()
    {
        capacity_temp_ = capacity_;
        read_index_temp_ = read_index_;
        write_index_temp_ = write_index_;
    }

    void restore_snapshot()
    {
        capacity_ = capacity_temp_;
        read_index_ = read_index_temp_;
        write_index_ = write_index_temp_;
    }

    std::string dump() const
    {
        if (!data())
        {
            return "";
        }

        std::stringstream ss;
        ss << std::hex << std::uppercase;
        for (auto i = 0; i < unread_length(); ++i)
        {
            char c = data()[i];
            std::bitset<8> b(c);
            if (((uint8_t)c) < 16)
            {
                ss << "0";
            }

            ss << b.to_ulong() << " ";
            if ((i + 1) % 8 == 0)
            {
                ss << "\n";
            }
        }

        return ss.str();
    }

    // for session do_read()
    size_t socket_read_length(size_t n)
    {
        static constexpr size_t kMaxCacheSize = 4 * 1024 * 1024;

        if (writable_bytes() >= n)
        {
            // if buffer has enough space for the next read for session
            return n;
        }

        move_unread_to_begin();

        if (writable_bytes() >= n)
        {
            return n;
        }

        if (capacity() > kMaxCacheSize)
        {
            throw std::runtime_error("session read buffer has reached limit, client "
                                     "is sending too much data");
        }

        // ensure read for this time
        grow(n);

        return n;
    }

    void socket_consume(size_t n)
    {
        if (n > writable_bytes())
        {
            throw std::runtime_error("session write_index is out of range");
        }

        write_index_ += n;
    }

    // prepend data
    void prepend(char *data, size_t size)
    {
        if (!data || !size)
        {
            return;
        }

        // 0 ---------- size ---------- read_index_
        if (read_index_ >= size)
        {
            // if the size of consumed data length is greater than the size of prepended data, then reuse the consumed memory
            read_index_ -= size;
            std::memcpy(data_ + read_index_, data, size);
            return;
        }

        auto available_bytes = writable_bytes();
        if (available_bytes >= size)
        {
            /**
             If the available size of the current buffer is greater than the size
             of the prepended data, simply shift the existing data to the right.
             */
            std::memmove(data_ + read_index_ + size, data_ + read_index_, unread_length());
            std::memcpy(data_ + read_index_, data, size);
            // read_index no need to change
            write_index_ += size;
            return;
        }

        auto offset = size - read_index_;
        if (available_bytes >= offset)
        {
            std::memmove(data_ + read_index_ + offset, data_ + read_index_, unread_length());
            read_index_ = 0;
            write_index_ += offset;
            std::memcpy(data_ + read_index_, data, size);
            return;
        }

        // The current buffer is unable to accommodate both the prepended data and the existing data
        size_t capacity = capacity_ << 1 + size;
        char *temp = new char[capacity];
        std::memcpy(temp, data, size);
        std::memcpy(temp + size, data_ + read_index_, unread_length());
        if (data_)
        {
            delete[] data_;
            data_ = nullptr;
        }
        capacity_ = capacity;
        read_index_ = 0;
        write_index_ = size + unread_length();
        data_ = temp;
    }

private:
    char *begin()
    {
        return data_;
    }
    const char *begin() const
    {
        return data_;
    }

    void ensure_writable_bytes(size_t n)
    {
        if (writable_bytes() < n)
        {
            grow(n);
        }
    }

    // grow the buffer to hold len bytes in written area
    void grow(size_t len)
    {
        auto available_bytes = writable_bytes();
        if (available_bytes < len)
        {
            // grow the capacity
            size_t capacity = (capacity_ << 1) + len;
            size_t unconsumed_length = unread_length();
            char *temp = new char[capacity];
            // copy the data that hasn't been consumed
            std::memcpy(temp, begin() + read_index_, unconsumed_length);
            read_index_ = 0;
            write_index_ = unconsumed_length;
            capacity_ = capacity;
            if (data_)
            {
                delete[] data_;
                data_ = nullptr;
            }
            data_ = temp;
        }
        else
        {
            move_unread_to_begin();
        }
    }

    void move_unread_to_begin()
    {
        // move unread data to the front, make space inside buffer
        size_t unconsumed_length = unread_length();
        std::memmove(begin(), begin() + read_index_, unconsumed_length);
        read_index_ = 0;
        write_index_ = unconsumed_length;
    }

private:
    char *data_{nullptr};
    size_t capacity_;
    size_t read_index_;
    size_t write_index_;

    size_t capacity_temp_{0};
    size_t read_index_temp_{0};
    size_t write_index_temp_{0};
};

} // namespace network