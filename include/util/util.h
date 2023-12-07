#pragma once
#include <chrono>

namespace util {
// get current timestamp in microsecond
uint64_t current_micros();
// get current timestamp in millisecond
uint64_t current_millis();
} // namespace util