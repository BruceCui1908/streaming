#pragma once

#include "frame.h"
#include "meta.h"
#include "network/flat_buffer.h"

namespace codec {

class track {
public:
  using ptr = std::shared_ptr<track>;

  track() = default;
  virtual ~track() = default;

  void set_bit_rate(uint32_t bit_rate) { bit_rate_ = bit_rate; }
  uint32_t bit_rate() const { return bit_rate_; }

  virtual Codec_Type get_codec() = 0;

  virtual void parse_config(const network::flat_buffer::ptr &) = 0;

  virtual void input_frame(const frame::ptr &fr) { translate_frame(fr); }

  void set_frame_translator(const frame_translator::ptr &ft) { ft_ = ft; }

private:
  void translate_frame(const frame::ptr &fr) {
    if (!fr) {
      spdlog::error("The frame_translator has not been set; therefore, the "
                    "frame cannot be translated.");
      return;
    }

    ft_->translate_frame(fr);
  }

private:
  uint32_t bit_rate_{0};
  frame_translator::ptr ft_;
};

class video_track : public track {
public:
  using ptr = std::shared_ptr<video_track>;

  virtual int get_video_height() const = 0;
  virtual int get_video_width() const = 0;
  virtual float get_video_fps() const = 0;
};

class audio_track : public track {
public:
  using ptr = std::shared_ptr<audio_track>;

  virtual uint32_t get_audio_sample_rate() = 0;
  virtual uint32_t get_audio_sample_bit() = 0;
  virtual uint8_t get_audio_channel() = 0;
};

} // namespace codec