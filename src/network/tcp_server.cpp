#include "tcp_server.h"

#include <csignal>
#include <stdexcept>

#include <spdlog/spdlog.h>

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

  spdlog::info("Successfully created {}", info());
}

void tcp_server::do_accept() {
  acceptor_.async_accept(
      [this](boost::system::error_code ec, tcp::socket socket) {
        // Check whether the server was stopped by a signal before this
        // completion handler had a chance to run.
        if (!acceptor_.is_open()) {
          spdlog::warn("acceptor closed in {}, about to return", info());
          return;
        }

        if (!ec) {
          spdlog::info("received connection");
        }

        do_accept();
      });
}

const std::string tcp_server::info() {
  return fmt::format("TCP server {}|{}", port_, ip_type_);
}

void tcp_server::start_signal_listener() {
  signals_.async_wait([this](boost::system::error_code, int signo) {
    spdlog::info("{} received signal {}", info(), signo);
    // close the acceptor
    acceptor_.close();
  });
}

} // namespace network