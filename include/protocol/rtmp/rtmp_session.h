#pragma once

#include "network/session.h"

namespace rtmp {
class rtmp_session : public network::session {
public:
  using ptr = std::shared_ptr<rtmp_session>;

  static ptr create(boost::asio::ip::tcp::socket);

private:
  rtmp_session(boost::asio::ip::tcp::socket);
};

} // namespace rtmp