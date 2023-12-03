#include "rtmp_session.h"

namespace rtmp {

rtmp_session::ptr rtmp_session::create(RTMP_CONSTRUCTOR_PARAMS) {
  return std::shared_ptr<rtmp_session>(
      new rtmp_session(std::move(sock), session_prefix, manager));
}

rtmp_session::rtmp_session(RTMP_CONSTRUCTOR_PARAMS)
    : session(std::move(sock), session_prefix, manager) {}

void rtmp_session::start() {
  spdlog::debug("rtmp session on [{}] started!", id());
  do_read();
}

void rtmp_session::stop() {
  network::session::stop();
  spdlog::debug("rtmp session on {} stopped!", id());
}
} // namespace rtmp