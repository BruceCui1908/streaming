#pragma once

#include "codec/track.h"
#include "rtmp_codec.h"

namespace rtmp {

class aac_rtmp_decoder : public rtmp_codec
{
public:
    using ptr = std::shared_ptr<aac_rtmp_decoder>;

    aac_rtmp_decoder(const codec::track::ptr &ptr)
        : rtmp_codec(ptr)
    {}

    void input_rtmp(rtmp_packet::ptr &) override;
};

} // namespace rtmp