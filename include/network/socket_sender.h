#pragma once

#include <memory>

namespace network {

class socket_sender
{
public:
    using ptr = std::shared_ptr<socket_sender>;

    socket_sender() = default;
    virtual ~socket_sender() = default;

    virtual void send(const char *, size_t, bool is_async = false, bool is_close = false) = 0;
};

} // namespace network