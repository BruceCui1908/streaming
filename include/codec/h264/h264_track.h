#pragma once

#include "codec/track.h"
#include "h264_frame.h"

namespace codec {

class h264_track : public video_track
{
public:
    using ptr = std::shared_ptr<h264_track>;

    int get_video_height() const override
    {
        return height_;
    }
    int get_video_width() const override
    {
        return width_;
    }
    float get_video_fps() const override
    {
        return fps_;
    }
    Codec_Type get_codec() override
    {
        return Codec_Type::CodecH264;
    }

    void parse_config(network::flat_buffer &) override;

    const std::string &get_sps() const
    {
        return sps_;
    }
    const std::string &get_pps() const
    {
        return pps_;
    }

private:
    void extract_bitstream_sps();
    void encapsulate_config_frame(const std::string &);

private:
    int height_{0};
    int width_{0};
    float fps_{0.0};
    std::string sps_;
    std::string pps_;
};

} // namespace codec