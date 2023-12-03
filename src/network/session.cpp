#include "session.h"

#include <spdlog/spdlog.h>
namespace network {

// session impl
session::session(boost::asio::ip::tcp::socket sock,
                 const std::string &session_prefix)
    : socket_{std::move(sock)},
      session_prefix_{session_prefix}, raw_fd_{socket_.native_handle()} {}

std::string session::id() const {
  return fmt::format("{}-fd({})", session_prefix_, raw_fd_);
}

// session_manager impl
session_manager::ptr session_manager::create(std::string prefix) {
  return std::shared_ptr<session_manager>(
      new session_manager(std::move(prefix)));
}

session_manager::session_manager(std::string session_prefix)
    : session_prefix_{std::move(session_prefix)} {}

std::string session_manager::generate_session_prefix() {
  static std::atomic_int64_t session_index{0};
  return fmt::format("{}-session({})", session_prefix_, ++session_index);
}

void session_manager::add(const session::ptr &session) {
  // TODO
}

} // namespace network