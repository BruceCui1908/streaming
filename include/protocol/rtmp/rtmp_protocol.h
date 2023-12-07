#pragma once

#include "network/buffer.h"
#include "network/flat_buffer.h"
#include "util/resource_pool.h"

namespace rtmp {
class rtmp_protocol {
public:
  virtual ~rtmp_protocol() = default;

protected:
  rtmp_protocol();

  void on_parse_rtmp(const char *, size_t);

private:
  const char *handle_C0C1(const char *, size_t);

private:
  // for sending rtmp packet
  util::resource_pool<network::buffer_raw>::ptr pool_;
  network::flat_buffer cache_data_;
  std::function<const char *(const char *, size_t)> next_step_func_;
};

} // namespace rtmp