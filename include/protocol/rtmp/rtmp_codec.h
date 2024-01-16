#pragma once

#include "codec/track.h"
#include "rtmp_packet.h"

namespace rtmp {

class rtmp_codec
{
public:
    using ptr = std::shared_ptr<rtmp_codec>;

    rtmp_codec(const codec::track::ptr &);

    virtual ~rtmp_codec() = default;

    virtual void input_rtmp(rtmp_packet::ptr) = 0;

    const codec::track::ptr get_track() const
    {
        return track_;
    }

private:
    codec::track::ptr track_;
};

} // namespace rtmp