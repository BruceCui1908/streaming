#pragma once

#include <memory>
#include <string>

#include "meta.h"

namespace http {

class http_header
{
public:
    using ptr = std::shared_ptr<http_header>;

    static constexpr char kGET_Method[] = "GET";
    static constexpr char kFlv[] = ".flv";

    static ptr build(const char *, size_t);

    Method method() const
    {
        return method_;
    }

    const std::string &app() const
    {
        return app_;
    }

    const std::string &stream() const
    {
        return stream_;
    }

    const std::string &vhost() const
    {
        return vhost_;
    }

    const std::string &token() const
    {
        return token_;
    }

private:
    http_header(const char *, size_t);

    void parse(const char *, size_t);

    size_t extract_streaming_info(const std::string &);

private:
    Method method_{INVALID};
    std::string app_;
    std::string stream_;
    std::string vhost_;
    std::string token_;
};
} // namespace http