#include "rtmp_codec.h"

namespace rtmp {
rtmp_codec::rtmp_codec(const codec::track::ptr &track_ptr)
    : track_{track_ptr}
{}

} // namespace rtmp