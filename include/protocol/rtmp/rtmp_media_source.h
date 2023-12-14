#pragma once

#include "media/media_source.h"
#include "rtmp_demuxer.h"

namespace rtmp {

typedef enum {
  Audio = 0,
  Video = 1,
} Track_Type;

class rtmp_media_source : public media::media_source {
public:
  using ptr = std::shared_ptr<rtmp_media_source>;

  static ptr create(const media::media_info::ptr &);

  void init_tracks(std::unordered_map<std::string, std::any> &);

  void process_av_packet(rtmp_packet::ptr);

private:
  rtmp_media_source(const media::media_info::ptr &);

  // timestamps for audio/video
  uint32_t track_stamps_[2] = {0};
  rtmp_demuxer::ptr demuxer_;
};

} // namespace rtmp