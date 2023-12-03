#include "tcp_server.h"

#include <csignal>
#include <stdexcept>

namespace network {

#define DEFAULT_LOCAL_IPV4 "0.0.0.0"
#define DEFAULT_LOCAL_IPV6 "0::0"

tcp_server::ptr tcp_server::create(boost::asio::io_context &io_context,
                                   const uint16_t port, ip_type ipType) {
  return std::shared_ptr<tcp_server>(new tcp_server(io_context, port, ipType));
}

tcp_server::tcp_server(boost::asio::io_context &io_context, const uint16_t port,
                       ip_type ipType)
    : server(port, sock_type::tcp, ipType), signals_{io_context},
      acceptor_{io_context} {

  // register ctrl-c and kill
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
  start_signal_listener();

  tcp::resolver resolver(io_context);
  auto result = resolver.resolve(ipType == ip_type::ipv4 ? DEFAULT_LOCAL_IPV4
                                                         : DEFAULT_LOCAL_IPV6,
                                 std::to_string(port));
  if (result.empty()) {
    throw std::invalid_argument("cannot resolve tcp address");
  }

  auto endpoint = result.begin()->endpoint();
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(tcp::acceptor::reuse_address(true));
  acceptor_.non_blocking(true);
  acceptor_.bind(endpoint);
  acceptor_.listen();

  raw_fd_ = acceptor_.native_handle();

  session_manager_ = session_manager::create(info());

  spdlog::info("{} created", info());
}

void tcp_server::do_accept() {
  if (!acceptor_.is_open()) {
    return;
  }

  acceptor_.async_accept(
      [this](boost::system::error_code ec, tcp::socket sock) {
        // Check whether the server was stopped by a signal before this
        // completion handler had a chance to run.
        if (!acceptor_.is_open()) {
          spdlog::warn("acceptor closed in {}, about to return", info());
          return;
        }

        if (!ec) {
          // create new session and start
          auto new_session = session_alloc_(std::move(sock));
          new_session->start();
        } else {
          spdlog::error("acceptor received error {}, msg = {}", ec.value(),
                        ec.message());
        }

        do_accept();
      });
}

void tcp_server::restart() { do_accept(); }

const std::string &tcp_server::info() {
  if (server_name_.empty()) {
    server_name_ =
        fmt::format("TCP[{}|{}|{}]", port_,
                    ip_type_ == ip_type::ipv4 ? "ipv4" : "ipv6", raw_fd_);
  }
  return server_name_;
}

void tcp_server::start_signal_listener() {
  signals_.async_wait([this](boost::system::error_code, int signo) {
    spdlog::info("{} received signal {}", info(), signo);
    // close the acceptor
    acceptor_.close();
    // close the session map
    session_manager_->stop_all();
  });
}

} // namespace network