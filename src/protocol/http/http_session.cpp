#include "http_session.h"

namespace http {

http_session::ptr http_session::create(SESSION_CONSTRUCTOR_PARAMS)
{
    return std::shared_ptr<http_session>(new http_session(std::move(sock), session_prefix, manager));
}

void http_session::start()
{
    spdlog::debug("http session on [{}] started!", id());
    do_read();
}

http_session::http_session(SESSION_CONSTRUCTOR_PARAMS)
    : session(std::move(sock), session_prefix, manager)
{}

void http_session::on_recv(network::flat_buffer &buf)
{
    ticker_.reset_time();

    try
    {
        // send the packet to the http parser
        on_parse_http(buf);
    }
    catch (const std::exception &ex)
    {
        spdlog::error("error received while parsing http, stop the rtmp sesssion, "
                      "session = {}, error = {}",
            id(), ex.what());
        shutdown();
    }
}
} // namespace http