#pragma once

#include "codec/track.h"
#include "rtmp_packet.h"

namespace rtmp {
class rtmp_codec {
public:
  using ptr = std::shared_ptr<rtmp_codec>;

  rtmp_codec(const codec::track::ptr &);

  virtual void input_rtmp(const rtmp_packet::ptr &);

private:
  codec::track::ptr track_;
};

} // namespace rtmp