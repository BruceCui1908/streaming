#pragma once

#include "aac_rtmp.h"
#include "h264_rtmp.h"
#include "media/demuxer.h"
#include "rtmp_packet.h"


#include <any>
#include <string>
#include <unordered_map>

namespace rtmp {

class rtmp_demuxer : public media::demuxer {
public:
  using ptr = std::shared_ptr<rtmp_demuxer>;

  void init_tracks_with_metadata(std::unordered_map<std::string, std::any> &);
  void init_video_track(int codecid, int bit_rate);
  void init_audio_track(int codecid, int sample_rate, int channels,
                        int sample_bit, int bit_rate);

  void input_rtmp(const rtmp_packet::ptr &);

private:
  double duration_{0.0};
  codec::video_track::ptr video_track_;
  codec::audio_track::ptr audio_track_;
  rtmp_codec::ptr audio_rtmp_decoder_;
  rtmp_codec::ptr video_rtmp_decoder_;
};

} // namespace rtmp