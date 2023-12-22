#pragma once

#include "flat_buffer.h"

#include <boost/asio.hpp>

#include <functional>
#include <mutex>
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

  friend class session_manager;

  static constexpr size_t kMaxBufferCacheSize = 8 * 1024;
  static constexpr size_t kSocketReadSize = kMaxBufferCacheSize / 2;

  virtual ~session();

  session(const session &) = delete;
  session &operator=(const session &) = delete;
  session(session &&) = delete;
  session &operator=(session &&) = delete;

  const std::string &id();
  /// destroy session
  void shutdown();
  virtual void start() = 0;

protected:
  session(SESSION_CONSTRUCTOR_PARAMS);

  /// Perform an asynchronous read operation.
  void do_read();
  void do_write(const char *, size_t, bool is_async = false,
                bool is_close = false);
  virtual void on_recv(flat_buffer &) = 0;

private:
  /// only session_manager can close the socket
  void stop();

protected:
  flat_buffer buffer_{kMaxBufferCacheSize};
  boost::asio::ip::tcp::socket socket_;
  std::string session_prefix_;
  int raw_fd_{-1};
  std::string id_;
  session_manager_ptr session_manager_;
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

  std::string generate_prefix();

private:
  session_manager(std::string);

private:
  std::string session_prefix_;
  std::mutex mtx_{};
  std::unordered_map<std::string, session::ptr> session_map_{};
};

} // namespace network