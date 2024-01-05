#include "http_protocol.h"

#include "media/media_source.h"
#include "util/util.h"

namespace http {

http_protocol::http_protocol()
{
    // initialize buffer
    pool_ = util::resource_pool<network::buffer_raw>::create();
}

void http_protocol::on_parse_http(network::flat_buffer &buf)
{
    const char *dummy = buf.data();
    const char *index = on_search_packet_tail(dummy, buf.unread_length());
    // cannot locate packet tail
    if (!index)
    {
        return;
    }

    auto header_len = index - dummy;
    on_search_http_handler(dummy, header_len);
    buf.safe_consume(header_len);
}

const char *http_protocol::on_search_packet_tail(const char *data, size_t size)
{
    if (!data || !size || size < 4)
    {
        throw std::runtime_error("http parser received invalid packet");
    }

    auto pos = std::strstr(data, kHttpPacketTail);
    return pos ? pos + 4 : nullptr;
}

void http_protocol::on_search_http_handler(const char *data, size_t size)
{
    if (!data || !size)
    {
        throw std::runtime_error("http parser received invalid packet");
    }

    try
    {
        header_ = http_flv_header::build(data, size);
    }
    catch (const std::exception &ex)
    {
        spdlog::error("failed to parse http header {}, error = {}", data, ex.what());
        send_response(Bad_Request, true);
        return;
    }

    using http_handler = void (http_protocol::*)();
    static std::unordered_map<Method, http_handler> handler_map = {{Method::GET, &http_protocol::on_http_get}};

    auto it = handler_map.find(header_->method());
    if (it == handler_map.end())
    {
        // send 405
        send_response(Method_Not_Allowed, true);
        return;
    }

    auto handler = it->second;
    try
    {
        (this->*handler)();
    }
    catch (const std::exception &ex)
    {
        spdlog::error("Error occurred while executing the HTTP handler., err = {}", ex.what());
        send_response(Internal_Server_Error, true);
    }
}

void http_protocol::on_http_get()
{
    // skip token validation

    if (header_->is_flv())
    {
        start_flv_muxing(get_session());
        return;
    }

    send_response(Unsupported_Media_Type, true);
}

void http_protocol::start_flv_muxing(network::session::ptr session_ptr)
{
    auto [media_src_ptr, is_found] = media::media_source::find(media::kRTMP_SCHEMA, header_->vhost(), header_->app(), header_->stream());

    if (!is_found)
    {
        send_response(Not_Found, true, k404Body, sizeof(k404Body) - 1, "text/html");
        return;
    }

    // send flv response header
    std::multimap<std::string, std::string> res_header{{"Cache-Control", "no-store"}};
    send_response(OK, false, nullptr, 0, "video/x-flv", res_header);

    // lazy initialization
    flv_muxer_ = flv::flv_muxer::create();
    auto rtmp_src_ptr = std::dynamic_pointer_cast<rtmp::rtmp_media_source>(media_src_ptr);
    flv_muxer_->start_muxing(this, std::move(session_ptr), rtmp_src_ptr, header_, header_->start_pts());
}

void http_protocol::send_response(code status, bool is_close, const char *http_body, size_t body_size, const char *content_type,
    const std::multimap<std::string, std::string> &headers)
{
    // prepare response header
    std::multimap<std::string, std::string> response{headers};
    response.emplace("server", kServerName);
    response.emplace("date", util::http_date());
    response.emplace("Connection", is_close ? "close" : "keep-alive");

    if (!is_close)
    {
        response.emplace("Keep-Alive", "timeout=20, max=100");
    }

    // default allow cross domain
    response.emplace("Access-Control-Allow-Origin", "*");
    response.emplace("Access-Control-Allow-Credentials", "true");

    bool has_response_body = http_body && body_size;
    if (has_response_body)
    {
        response.emplace("Content-Length", std::to_string(body_size));
    }

    if (!content_type)
    {
        content_type = "text/plain";
    }
    response.emplace("Content-Type", fmt::format("{}; charset=utf-8", content_type));

    // send http header
    std::string res;
    res.reserve(256);
    res += "HTTP/1.1 ";
    res += std::to_string(status);
    res += ' ';
    res += code_to_msg(status);
    res += kHttpLineBreak;

    for (auto &pr : response)
    {
        res += pr.first;
        res += ": ";
        res += pr.second;
        res += kHttpLineBreak;
    }

    res += kHttpLineBreak;

    // if no http body, just close
    send(res.c_str(), res.size(), true, is_close);

    if (has_response_body)
    {
        send(http_body, body_size, true, close);
    }
}

} // namespace http