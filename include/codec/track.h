#pragma once

#include "frame.h"
#include "meta.h"
#include <memory>

namespace codec {

class track {
public:
  using ptr = std::shared_ptr<track>;

  track() = default;
  virtual ~track() = default;

  void set_bit_rate(int bit_rate) { bit_rate_ = bit_rate; }

  const int bit_rate() const { return bit_rate_; }

  virtual Codec_Type get_codec() = 0;
  virtual void input_frame(const frame::ptr &) = 0;

private:
  int bit_rate_{0};
};

class video_track : public track {
public:
  using ptr = std::shared_ptr<video_track>;

  virtual int get_video_height() const = 0;
  virtual int get_video_width() const = 0;
  virtual double get_video_fps() const = 0;
};

class audio_track : public track {
public:
  using ptr = std::shared_ptr<audio_track>;

  virtual int get_audio_sample_rate() = 0;
  virtual int get_audio_sample_bit() = 0;
  virtual int get_audio_channel() = 0;
};

} // namespace codec