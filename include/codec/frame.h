#pragma once

#include "meta.h"
#include "network/flat_buffer.h"

#include <memory>

namespace codec {

class frame
{
public:
    using ptr = std::shared_ptr<frame>;

    frame(Codec_Type codec_id)
        : codec_id_{codec_id}
    {}

    virtual ~frame() = default;

    void set_dts(uint32_t ds)
    {
        dts_ = ds;
    }
    uint32_t dts() const
    {
        return dts_;
    }

    void set_pts(uint32_t ps)
    {
        pts_ = ps;
    }
    uint32_t pts() const
    {
        return pts_ ? pts_ : dts_;
    }

    // h264 is 4, aac is 7
    void set_prefix_size(uint8_t size)
    {
        prefix_size_ = size;
    }
    size_t prefix_size() const
    {
        return prefix_size_;
    }

    void set_data(const network::flat_buffer::ptr &ptr)
    {
        if (!ptr)
        {
            throw std::runtime_error("Unable to assign an empty buffer to the frame.");
        }
        buf_ = ptr;
    }
    const network::flat_buffer::ptr &data() const
    {
        if (!buf_)
        {
            throw std::runtime_error("The buffer within the frame has been lost");
        }
        return buf_;
    }

    Codec_Type codec_id() const
    {
        return codec_id_;
    }

    virtual bool is_key_frame() const = 0;
    virtual bool is_config_frame() const = 0;

private:
    Codec_Type codec_id_;
    uint32_t dts_{0};
    uint32_t pts_{0};
    uint8_t prefix_size_{0};
    network::flat_buffer::ptr buf_;
};

class frame_translator
{
public:
    using ptr = std::shared_ptr<frame_translator>;

    frame_translator() = default;
    virtual ~frame_translator() = default;

    virtual void translate_frame(const frame &) = 0;
};

} // namespace codec