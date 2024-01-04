#pragma once

#include "rtmp/rtmp_media_source.h"
#include "network/socket_sender.h"
#include "util/resource_pool.h"
#include "network/buffer.h"
#include "flv_header.h"

namespace flv {

class flv_muxer : public std::enable_shared_from_this<flv_muxer>
{
public:
    using ptr = std::shared_ptr<flv_muxer>;

    static ptr create();

    void start_muxing(network::socket_sender *, rtmp::rtmp_media_source::ptr &, uint32_t start_pts);

private:
    flv_muxer();

    network::buffer_raw::ptr prepare_flv_header();

    void write_flv(network::socket_sender *, tag_type, const char *, size_t, uint32_t time_stamp = 0);

    network::buffer_raw::ptr prepare_flv_tag_header(tag_type, size_t, uint32_t time_stamp = 0);

private:
    util::resource_pool<network::buffer_raw>::ptr pool_;
};

} // namespace flv