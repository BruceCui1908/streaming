#include "session.h"

namespace network {

session::session(boost::asio::ip::tcp::socket sock)
    : socket_{std::move(sock)} {}

} // namespace network