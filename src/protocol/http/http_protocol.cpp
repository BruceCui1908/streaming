#include "http_protocol.h"

#include <map>
#include "util/util.h"
namespace http {

http_protocol::http_protocol()
{
    // initialize buffer
    pool_ = util::resource_pool<network::buffer_raw>::create();
}

void http_protocol::on_parse_http(network::flat_buffer &buf)
{
    spdlog::info("http server received data\n {}", buf.data());

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

    header_ = http_flv_header::build(data, size);

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
    (this->*handler)();
}

void http_protocol::on_http_get()
{
    // skip token validation
    spdlog::info("in http get handler");
    // TODO
}

void http_protocol::send_response(code status, bool is_close)
{
    std::multimap<std::string, std::string> response{};
    response.emplace("server", kServerName);
    response.emplace("date", util::http_date());
    response.emplace("Connection", is_close ? "close" : "keep-alive");

    if (!is_close)
    {
        response.emplace("Keep-Alive", "timeout=10, max=100");
    }

    // default allow cross domain
    response.emplace("Access-Control-Allow-Origin", "*");
    response.emplace("Access-Control-Allow-Credentials", "true");

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

    send(res.c_str(), res.size(), true, true);

    // TODO
}

} // namespace http