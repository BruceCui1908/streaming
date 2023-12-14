#pragma once

#include "codec/track.h"
#include "rtmp_codec.h"

namespace rtmp {
class h264_rtmp_decoder : public rtmp_codec {
public:
  using ptr = std::shared_ptr<h264_rtmp_decoder>;

  h264_rtmp_decoder(const codec::track::ptr &ptr) : rtmp_codec(ptr) {}
};

} // namespace rtmp