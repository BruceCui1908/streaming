#include "http_protocol.h"

namespace http {

http_protocol::http_protocol()
{
    // initialize buffer
    pool_ = util::resource_pool<network::buffer_raw>::create();
}

void http_protocol::on_parse_http(network::flat_buffer &buf)
{
    spdlog::info("http server received data\n {}", buf.data());
}

} // namespace http