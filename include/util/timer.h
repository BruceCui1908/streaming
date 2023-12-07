#pragma once

#include "util.h"

namespace util {
class ticker {
public:
  ticker() { begin_ = created_ = util::current_millis(); }

  ~ticker() = default;

  uint64_t elapsed_time() { return util::current_millis() - begin_; }

  uint64_t created_time() { return util::current_millis() - created_; }

  void reset_time() { begin_ = util::current_millis(); }

private:
  uint64_t begin_;
  uint64_t created_;
};

class bytes_speed {
public:
  bytes_speed() = default;
  ~bytes_speed() = default;

  bytes_speed &operator+=(size_t bytes) {
    bytes_ += bytes;
    if (bytes_ > 1024 * 1024) {
      compute_speed();
    }
    return *this;
  }

  int get_speed() {
    if (ticker_.elapsed_time() < 1000) {
      return speed_;
    }

    return compute_speed();
  }

private:
  int compute_speed() {
    auto elapsed = ticker_.elapsed_time();
    if (!elapsed) {
      return speed_;
    }

    speed_ = (int)(bytes_ * 1000 / elapsed);
    ticker_.reset_time();
    bytes_ = 0;
    return speed_;
  }

private:
  int speed_{0};
  size_t bytes_{0};
  ticker ticker_;
};

} // namespace util
