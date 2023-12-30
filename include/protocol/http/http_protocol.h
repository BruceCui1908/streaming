#pragma once

#include "network/flat_buffer.h"
#include "network/buffer.h"
#include "util/resource_pool.h"

namespace http {

class http_protocol
{
public:
    virtual ~http_protocol() = default;

protected:
    http_protocol();

    void on_parse_http(network::flat_buffer &);

private:
    // for sending rtmp packet
    util::resource_pool<network::buffer_raw>::ptr pool_;
};

} // namespace http