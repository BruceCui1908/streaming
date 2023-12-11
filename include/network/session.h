#pragma once

#include <boost/asio.hpp>

#include <spdlog/spdlog.h>

#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

#define SESSION_CONSTRUCTOR_PARAMS                                             \
  boost::asio::ip::tcp::socket sock, const std::string &session_prefix,        \
      const network::session_manager_ptr &manager

namespace network {

typedef enum { operation_cancelled = 125 } err;

class session_manager;
using session_manager_ptr = std::shared_ptr<session_manager>;

class session : public std::enable_shared_from_this<session> {
public:
  using ptr = std::shared_ptr<session>;
  using err_cb = std::function<void(const session_manager_ptr &)>;

  virtual ~session();

  session(const session &) = delete;
  session &operator=(const session &) = delete;
  session(session &&) = delete;
  session &operator=(session &&) = delete;

  const std::string &id();

  virtual void start() = 0;
  virtual void stop();

protected:
  session(SESSION_CONSTRUCTOR_PARAMS);

  /// Perform an asynchronous read operation.
  void do_read();

  void do_write(const char *, size_t, bool is_async = false,
                bool is_close = false);

  /// @brief  data read from the socket, if the operation is asynchronous, then
  /// copy the data into new buffer
  /// @param
  /// @param
  virtual void on_recv(char *, size_t) = 0;

protected:
  /// Buffer for incoming data.
  std::array<char, 8192> buffer_;
  boost::asio::ip::tcp::socket socket_;
  std::string session_prefix_;
  int raw_fd_{-1};
  std::string id_;
  session_manager_ptr session_manager_;
  uint64_t total_bytes_{0};
};

class session_manager : public std::enable_shared_from_this<session_manager> {
public:
  using ptr = std::shared_ptr<session_manager>;

  static ptr create(std::string);

  ~session_manager();
  session_manager(const session_manager &) = delete;
  session_manager &operator=(const session_manager &) = delete;
  session_manager(session_manager &&) = delete;
  session_manager &operator=(session_manager &&) = delete;

  void add(const session::ptr &);
  void stop(const session::ptr &);
  void stop_all();

  std::string generate_session_prefix();

private:
  session_manager(std::string);

private:
  std::string session_prefix_;
  std::mutex mtx_{};
  std::unordered_map<std::string, session::ptr> session_map_{};
};

} // namespace network