#pragma once

#include "rtmp/rtmp_media_source.h"
#include "network/socket_sender.h"

namespace flv {

class flv_muxer : public std::enable_shared_from_this<flv_muxer>
{
public:
    using ptr = std::shared_ptr<flv_muxer>;

    static ptr create();

    void start_muxing(network::socket_sender *, rtmp::rtmp_media_source::ptr &, uint32_t start_pts);

private:
    flv_muxer() = default;
};

} // namespace flv