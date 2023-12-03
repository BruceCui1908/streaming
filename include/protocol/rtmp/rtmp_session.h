#pragma once

#include "network/session.h"

namespace rtmp {

#define RTMP_CONSTRUCTOR_PARAMS SESSION_CONSTRUCTOR_PARAMS

class rtmp_session : public network::session {
public:
  using ptr = std::shared_ptr<rtmp_session>;

  static ptr create(RTMP_CONSTRUCTOR_PARAMS);

  ~rtmp_session() override = default;

  void start() override;
  void stop() override;

private:
  rtmp_session(RTMP_CONSTRUCTOR_PARAMS);
};

} // namespace rtmp