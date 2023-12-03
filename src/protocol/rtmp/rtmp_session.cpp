#include "rtmp_session.h"

namespace rtmp {

rtmp_session::ptr rtmp_session::create(boost::asio::ip::tcp::socket socket,
                                       const std::string &session_prefix) {
  return std::shared_ptr<rtmp_session>(
      new rtmp_session(std::move(socket), session_prefix));
}

rtmp_session::rtmp_session(boost::asio::ip::tcp::socket socket,
                           const std::string &session_prefix)
    : session(std::move(socket), session_prefix) {}

} // namespace rtmp