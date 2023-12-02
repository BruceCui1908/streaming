#include "server.h"

namespace network {

server::server(uint16_t port, sock_type sockType, ip_type ipType)
    : port_{port}, sock_type_{sockType}, ip_type_{ipType} {}
} // namespace network