#include "rtmp_session.h"
namespace rtmp {

rtmp_session::ptr rtmp_session::create(RTMP_CONSTRUCTOR_PARAMS)
{
    return std::shared_ptr<rtmp_session>(new rtmp_session(std::move(sock), session_prefix, manager));
}

rtmp_session::rtmp_session(RTMP_CONSTRUCTOR_PARAMS)
    : session(std::move(sock), session_prefix, manager)
{}

void rtmp_session::start()
{
    spdlog::debug("rtmp session on [{}] started!", id());
    do_read();
}

void rtmp_session::send(const char *data, size_t size, bool is_async, bool is_close)
{
    network::session::do_write(data, size, is_async, is_close);
}

void rtmp_session::on_recv(network::flat_buffer &buf)
{
    ticker_.reset_time();

    try
    {
        // send the packet to the rtmp parser
        on_parse_rtmp(buf);
    }
    catch (const std::exception &ex)
    {
        spdlog::error("error received while parsing rtmp, stop the rtmp sesssion, "
                      "session = {}, error = {}",
            id(), ex.what());
        shutdown();
    }
}

} // namespace rtmp