#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string>

namespace network {
typedef enum {
  tcp = 0,
  udp = 1,
} sock_type;

typedef enum {
  ipv4 = 0,
  ipv6 = 1,
} ip_type;

class server : public std::enable_shared_from_this<server> {
public:
  virtual ~server() = default;
  server(const server &) = delete;
  server &operator=(const server &) = delete;
  server(server &&) = delete;
  server &operator=(server &&) = delete;

  virtual const std::string info() = 0;

protected:
  server(uint16_t, sock_type, ip_type);

protected:
  uint16_t port_;
  sock_type sock_type_;
  ip_type ip_type_;
};

} // namespace network