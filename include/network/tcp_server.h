#pragma once

#include "server.h"

#include <boost/asio.hpp>

namespace network {

using boost::asio::ip::tcp;

class tcp_server : public server {
public:
  using ptr = std::shared_ptr<tcp_server>;

  static ptr create(boost::asio::io_context &, const uint16_t,
                    ip_type ipType = ip_type::ipv4);

  const std::string info() override;

  void start() override {
    start_signal_listener();

    do_accept();
  }

private:
  tcp_server(boost::asio::io_context &, const uint16_t, ip_type ipType);

  // callback after receiving termination signal
  void start_signal_listener();

  void do_accept();

private:
  /// The signal_set is used to register for process termination notifications.
  boost::asio::signal_set signals_;

  /// Acceptor used to listen for incoming connections.
  tcp::acceptor acceptor_;
};

} // namespace network