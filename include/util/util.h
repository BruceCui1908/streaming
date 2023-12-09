#pragma once
#include <chrono>

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

} // namespace util