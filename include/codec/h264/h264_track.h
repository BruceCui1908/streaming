#pragma once

#include "codec/track.h"

namespace codec {

class h264_track : public video_track {
public:
  using ptr = std::shared_ptr<h264_track>;

  int get_video_height() const override;
  int get_video_width() const override;
  double get_video_fps() const override;
  Codec_Type get_codec() override;

private:
  int height_{0};
  int width_{0};
  double fps_{0.0};
};

} // namespace codec