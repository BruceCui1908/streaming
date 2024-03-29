#pragma once

#include "meta.h"

#include <spdlog/spdlog.h>

#include <memory>
#include <string>

namespace http {

class http_flv_header
{
public:
    using ptr = std::shared_ptr<http_flv_header>;

    static constexpr char kGET_Method[] = "GET";
    static constexpr char kFlv[] = ".flv";
    static constexpr char kStart_Pts[] = "startpts=";

    static ptr build(const char *, size_t);

    Method method() const
    {
        return method_;
    }

    const std::string &app() const
    {
        return app_;
    }

    const std::string &stream_id() const
    {
        return stream_id_;
    }

    const std::string &vhost() const
    {
        return vhost_;
    }

    const std::string &token() const
    {
        return token_;
    }

    uint32_t start_pts() const
    {
        return start_pts_;
    }

    bool is_flv() const
    {
        return is_flv_;
    }

    const std::string &info()
    {
        if (info_.empty())
        {
            info_ = fmt::format("{}|{}|{}", app_, vhost_, stream_id_);
        }
        return info_;
    }

private:
    http_flv_header(const char *, size_t);

    void parse(const char *, size_t);

    size_t extract_streaming_info(const std::string &);

private:
    Method method_{Method::INVALID};
    std::string app_;
    std::string stream_id_;
    std::string vhost_;
    std::string token_;
    bool is_flv_{false};
    uint32_t start_pts_{0};
    std::string info_;
};
} // namespace http