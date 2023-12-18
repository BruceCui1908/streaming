#pragma once

#include "network/session.h"
#include "rtmp_protocol.h"
#include "util/timer.h"

namespace rtmp {

#define RTMP_CONSTRUCTOR_PARAMS SESSION_CONSTRUCTOR_PARAMS

class rtmp_session : public network::session, public rtmp_protocol {
public:
  using ptr = std::shared_ptr<rtmp_session>;

  static ptr create(RTMP_CONSTRUCTOR_PARAMS);

  ~rtmp_session() override = default;

  void start() override;

  void send(const char *, size_t, bool is_async = false,
            bool is_close = false) override;

private:
  rtmp_session(RTMP_CONSTRUCTOR_PARAMS);

  void on_recv(network::flat_buffer &) override;

private:
  std::function<const char *(const char *, size_t)> next_step_fun_;

  // ticker
  util::ticker ticker_;
};

} // namespace rtmp