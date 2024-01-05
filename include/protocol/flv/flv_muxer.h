#pragma once

#include "rtmp/rtmp_media_source.h"
#include "network/socket_sender.h"
#include "network/session.h"
#include "util/resource_pool.h"
#include "network/buffer.h"
#include "flv_header.h"
#include "http/http_flv_header.h"
#include "media/packet_dispatcher.h"

namespace flv {

class flv_muxer : public std::enable_shared_from_this<flv_muxer>
{
public:
    using ptr = std::shared_ptr<flv_muxer>;
    using client_reader_ptr = media::client_reader<rtmp::rtmp_packet::ptr>::ptr;

    static ptr create();

    void start_muxing(network::socket_sender *, network::session::ptr, rtmp::rtmp_media_source::ptr &, const http::http_flv_header::ptr &,
        uint32_t start_pts);

    ~flv_muxer();

private:
    flv_muxer();

    network::buffer_raw::ptr prepare_flv_header();

    void write_flv(network::socket_sender *, tag_type, const char *, size_t, uint32_t time_stamp = 0);

    network::buffer_raw::ptr prepare_flv_tag_header(tag_type, size_t, uint32_t time_stamp = 0);

private:
    util::resource_pool<network::buffer_raw>::ptr pool_;
    client_reader_ptr client_reader_ptr_;
};

} // namespace flv