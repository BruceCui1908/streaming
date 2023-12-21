#pragma once

#include "codec/track.h"

namespace codec {

class aac_track : public audio_track {
public:
  using ptr = std::shared_ptr<aac_track>;

  int get_audio_sample_rate() override;
  int get_audio_sample_bit() override;
  int get_audio_channel() override;
  Codec_Type get_codec() override;

  void parse_config(const network::flat_buffer::ptr &) override;

  // TODO
  void input_frame(const frame::ptr &) override {}

private:
  std::string cfg_;
  int channel_{0};
  int sample_rate_{0};
  int sample_bit_{16};
};

} // namespace codec