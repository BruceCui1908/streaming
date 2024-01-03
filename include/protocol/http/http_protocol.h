#pragma once

#include "network/flat_buffer.h"
#include "network/buffer.h"
#include "util/resource_pool.h"
#include "http_flv_header.h"

namespace http {

class http_protocol
{
public:
    static constexpr char kHttpLineBreak[] = "\r\n";

    static constexpr char kHttpPacketTail[] = "\r\n\r\n";

    static constexpr char kServerName[] = "OBS-Streaming";

    virtual ~http_protocol() = default;

protected:
    http_protocol();

    void on_parse_http(network::flat_buffer &);

    virtual void send(const char *, size_t, bool is_async = false, bool is_close = false) = 0;

private:
    const char *on_search_packet_tail(const char *, size_t);

    void on_search_http_handler(const char *, size_t);

    void on_http_get();

    void send_response(code, bool is_close);

private:
    http_flv_header::ptr header_;
    // for sending rtmp packet
    util::resource_pool<network::buffer_raw>::ptr pool_;
};

} // namespace http