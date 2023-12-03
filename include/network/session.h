#pragma once

#include <boost/asio.hpp>

#include <memory>
#include <string>
#include <type_traits>

namespace network {

class session : public std::enable_shared_from_this<session> {
public:
  using ptr = std::shared_ptr<session>;

  virtual ~session() = default;

  session(const session &) = delete;
  session &operator=(const session &) = delete;
  session(session &&) = delete;
  session &operator=(session &&) = delete;

  std::string id() const;

protected:
  session(boost::asio::ip::tcp::socket, const std::string &);

protected:
  boost::asio::ip::tcp::socket socket_;
  std::string session_prefix_;
  int raw_fd_{-1};
};

class session_manager : public std::enable_shared_from_this<session_manager> {
public:
  using ptr = std::shared_ptr<session_manager>;

  static ptr create(std::string);

  void add(const session::ptr &);

  std::string generate_session_prefix();

private:
  session_manager(std::string);

private:
  std::string session_prefix_;
};

} // namespace network