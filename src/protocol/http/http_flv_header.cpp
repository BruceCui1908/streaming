#include "http_flv_header.h"

#include "media/media_info.h"
#include <tuple>

namespace http {

http_flv_header::ptr http_flv_header::build(const char *data, size_t size)
{
    return std::shared_ptr<http_flv_header>(new http_flv_header(data, size));
}

http_flv_header::http_flv_header(const char *data, size_t size)
{
    parse(data, size);
}

void http_flv_header::parse(const char *data, size_t size)
{
    if (!data || !size)
    {
        throw std::runtime_error("http header is invalid");
    }

    std::string temp(data, size);
    auto info_end = extract_streaming_info(temp);
}

static std::tuple<bool, size_t, std::string> extract_param(const std::string &str, const char *start_label, const char *end_label,
    size_t offset = 0, size_t step_size = 1, const char *optional_end_label = nullptr, bool fail = true)
{
    if (str.empty() || !start_label || !end_label || offset < 0 || !step_size)
    {
        throw std::runtime_error("params cannot be empty in extract_param()");
    }

    size_t start_index, end_index = -1;

    start_index = str.find(start_label, offset);
    if (start_index == std::string::npos)
    {
        goto FAIL;
    }

    start_index += step_size;

    end_index = str.find(end_label, start_index);
    if (end_index != std::string::npos)
    {
        goto SUCCESS;
    }

    if (!optional_end_label)
    {
        goto FAIL;
    }

    end_index = str.find(optional_end_label, start_index);
    if (end_index != std::string::npos)
    {
        goto SUCCESS;
    }

FAIL:
    if (fail)
    {
        throw std::invalid_argument("cannot find end_label");
    }
    return {false, -1, ""};

SUCCESS:
    return {true, end_index, std::move(str.substr(start_index, end_index - start_index))};
}

size_t http_flv_header::extract_streaming_info(const std::string &temp)
{
    if (temp.empty())
    {
        throw std::runtime_error("cannot extract streaming info from empty http header");
    }

    // parse http method
    auto start_index = temp.find(" ");
    if (start_index == std::string::npos)
    {
        throw std::invalid_argument("cannot find http method");
    }
    std::string method = temp.substr(0, start_index);
    if (method == kGET_Method)
    {
        method_ = Method::GET;
    }

    bool is_found{false};
    size_t offset{0};
    std::string param;
    // extract app
    std::tie(is_found, offset, param) = extract_param(temp, "/", "/", start_index);
    app_ = std::move(param);

    // extract stream
    std::tie(is_found, offset, param) = extract_param(temp, "/", kFlv, offset);
    stream_ = std::move(param);
    is_flv_ = true;

    // extract vhost
    std::tie(is_found, offset, param) =
        extract_param(temp, media::media_info::kVhostParam, "&", offset, sizeof(media::media_info::kVhostParam) - 1);
    vhost_ = std::move(param);

    // extract token
    std::tie(is_found, offset, param) =
        extract_param(temp, media::media_info::kTokenParam, "&", offset, sizeof(media::media_info::kVhostParam) - 1, " ");
    token_ = std::move(param);

    // extract startpts
    std::tie(is_found, offset, param) = extract_param(temp, kStart_Pts, " ", offset, sizeof(kStart_Pts) - 1, "&", false);
    if (is_found)
    {
        start_pts_ = std::stoul(std::move(param));
    }

    return offset;
}

} // namespace http