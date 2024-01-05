#pragma once

#include "network/flat_buffer.h"
#include "network/buffer.h"
#include "network/socket_sender.h"
#include "network/session_receiver.h"
#include "util/resource_pool.h"
#include "http_flv_header.h"
#include "flv/flv_muxer.h"

#include <map>

namespace http {

class http_protocol : public network::socket_sender, public network::session_receiver
{
public:
    static constexpr char kHttpLineBreak[] = "\r\n";

    static constexpr char kHttpPacketTail[] = "\r\n\r\n";

    static constexpr char kServerName[] = "OBS-Streaming";

    virtual ~http_protocol() = default;

protected:
    http_protocol();

    void on_parse_http(network::flat_buffer &);

private:
    const char *on_search_packet_tail(const char *, size_t);

    void on_search_http_handler(const char *, size_t);

    void on_http_get();

    void send_response(code, bool is_close, const char *http_body = nullptr, size_t body_size = 0, const char *content_type = nullptr,
        const std::multimap<std::string, std::string> &headers = {});

private:
    void start_flv_muxing(network::session::ptr);

private:
    http_flv_header::ptr header_{nullptr};

    flv::flv_muxer::ptr flv_muxer_{nullptr};

    // for sending rtmp packet
    util::resource_pool<network::buffer_raw>::ptr pool_;
};

} // namespace http