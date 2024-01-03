#include "flv_muxer.h"

namespace flv {

flv_muxer::ptr flv_muxer::create()
{
    return std::shared_ptr<flv_muxer>(new flv_muxer);
}

void flv_muxer::start_muxing(network::socket_sender *sender, rtmp::rtmp_media_source::ptr &rtmp_src_ptr, uint32_t start_pts)
{
    // TODO
}

} // namespace flv