#include "rtmp_session.h"

namespace rtmp {

rtmp_session::ptr rtmp_session::create(boost::asio::ip::tcp::socket socket) {
  return std::shared_ptr<rtmp_session>(new rtmp_session(std::move(socket)));
}

rtmp_session::rtmp_session(boost::asio::ip::tcp::socket socket)
    : session(std::move(socket)) {}

} // namespace rtmp