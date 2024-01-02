#include "http_header.h"

#include "media/media_info.h"

namespace http {

http_header::ptr http_header::build(const char *data, size_t size)
{
    return std::shared_ptr<http_header>(new http_header(data, size));
}

http_header::http_header(const char *data, size_t size)
{
    parse(data, size);
}

void http_header::parse(const char *data, size_t size)
{
    if (!data || !size)
    {
        throw std::runtime_error("http header is invalid");
    }

    std::string temp(data, size);
    auto info_end = extract_streaming_info(temp);
}

size_t http_header::extract_streaming_info(const std::string &temp)
{
    if (temp.empty())
    {
        throw std::runtime_error("cannot extract streaming info from empty http header");
    }

    // parse http method
    auto offset = temp.find(' ');
    if (offset == std::string::npos)
    {
        throw std::runtime_error("cannot find http method");
    }
    std::string method = temp.substr(0, offset);
    if (method == kGET_Method)
    {
        method_ = Method::GET;
    }

    // parse app
    auto app_start = temp.find('/', offset);
    if (app_start == std::string::npos)
    {
        throw std::runtime_error("cannot find app start");
    }
    app_start++;
    auto app_end = temp.find('/', app_start);
    if (app_end == std::string::npos)
    {
        throw std::runtime_error("cannot find app end");
    }
    app_ = std::move(temp.substr(app_start, app_end - app_start));

    // parse stream
    auto stream_start = temp.find('/', app_end);
    if (stream_start == std::string::npos)
    {
        throw std::runtime_error("cannot find stream start");
    }
    stream_start++;
    auto stream_end = temp.find(kFlv, stream_start);
    if (stream_end == std::string::npos || stream_end == stream_start)
    {
        throw std::runtime_error("invalid flv stream");
    }
    stream_ = std::move(temp.substr(stream_start, stream_end - stream_start));

    // parse vhost
    auto vhost_start = temp.find(media::media_info::kVhostParam, stream_end);
    if (vhost_start == std::string::npos)
    {
        throw std::runtime_error("cannot find vhost start");
    }
    vhost_start += sizeof(media::media_info::kVhostParam) - 1;
    auto vhost_end = temp.find('&', vhost_start);
    if (vhost_end == std::string::npos)
    {
        throw std::runtime_error("cannot find vhost end");
    }
    vhost_ = std::move(temp.substr(vhost_start, vhost_end - vhost_start));

    // parse token
    auto token_start = temp.find(media::media_info::kTokenParam, vhost_end);
    if (token_start == std::string::npos)
    {
        throw std::runtime_error("cannot find token start");
    }
    token_start += sizeof(media::media_info::kTokenParam) - 1;
    auto token_end = temp.find(' ', token_start);
    token_ = std::move(temp.substr(token_start, token_end - token_start));

    return token_end;
}

} // namespace http