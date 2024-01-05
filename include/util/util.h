#pragma once

#include <chrono>
#include <string>
#include <type_traits>
#include <memory>

namespace util {
// get current timestamp in microsecond
uint64_t current_micros();
// get current timestamp in millisecond
uint64_t current_millis();

// uint8_t[4]
uint32_t load_le32(const void *);
// uint8_t[3]
uint32_t load_be24(const void *);
// uint8_t[4]
uint32_t load_be32(const void *);

void set_be24(void *, uint32_t);
void set_be32(void *, uint32_t);
void set_le32(void *, uint32_t);

std::string http_date();

template<typename T>
class is_shared_ptr : public std::false_type
{};

template<typename T>
class is_shared_ptr<std::shared_ptr<T>> : public std::true_type
{};

template<typename T>
inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;
} // namespace util