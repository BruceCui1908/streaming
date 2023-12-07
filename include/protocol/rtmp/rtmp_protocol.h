#pragma once

#include "network/buffer.h"
#include "network/flat_buffer.h"
#include "rtmp_packet.h"
#include "util/resource_pool.h"

namespace rtmp {
class rtmp_protocol {
public:
  virtual ~rtmp_protocol() = default;

protected:
  rtmp_protocol();

  void on_parse_rtmp(const char *, size_t);

  virtual void send(const char *, size_t) = 0;

private:
  const char *handle_C0C1(const char *, size_t);
  const char *handle_C2(const char *, size_t);
  const char *handle_rtmp(const char *, size_t);

private:
  // P13
  int chunk_stream_id_{0};

  // for sending rtmp packet
  util::resource_pool<network::buffer_raw>::ptr pool_;
  network::flat_buffer cache_data_;
  std::function<const char *(const char *, size_t)> next_step_func_;
};

} // namespace rtmp