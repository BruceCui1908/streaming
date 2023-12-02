#pragma once

#include <boost/asio.hpp>

#include <memory>

namespace network {

class session : public std::enable_shared_from_this<session> {
public:
  using ptr = std::shared_ptr<session>;

  virtual ~session() = default;

  session(const session &) = delete;
  session &operator=(const session &) = delete;
  session(session &&) = delete;
  session &operator=(session &&) = delete;

protected:
  session(boost::asio::ip::tcp::socket);

protected:
  boost::asio::ip::tcp::socket socket_;
};

} // namespace network