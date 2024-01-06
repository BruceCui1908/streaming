#pragma once

#include <memory>

namespace network {

enum class Sock_Type
{
    tcp = 0,
    udp = 1,
};

enum class Ip_Type
{
    ipv4 = 0,
    ipv6 = 1,
};

class server : public std::enable_shared_from_this<server>
{
public:
    virtual ~server() = default;
    server(const server &) = delete;
    server &operator=(const server &) = delete;
    server(server &&) = delete;
    server &operator=(server &&) = delete;

    virtual const std::string &info() = 0;
    virtual void restart() = 0;

protected:
    server(uint16_t port, Sock_Type sock_type, Ip_Type ip_type)
        : port_{port}
        , sock_type_{sock_type}
        , ip_type_{ip_type}
    {}

protected:
    uint16_t port_;
    Sock_Type sock_type_;
    Ip_Type ip_type_;
};

} // namespace network