#pragma once

#include "aac_frame.h"
#include "codec/track.h"

namespace codec {

static constexpr size_t kAdtsHeaderLength = 7;

class aac_track : public audio_track {
public:
  using ptr = std::shared_ptr<aac_track>;

  uint32_t get_audio_sample_rate() override { return sample_rate_; }
  uint32_t get_audio_sample_bit() override { return sample_bit_; }
  uint8_t get_audio_channel() override { return channel_; }
  Codec_Type get_codec() override { return Codec_Type::CodecAAC; }

  void parse_config(const network::flat_buffer::ptr &) override;

  void input_frame(const frame::ptr &) override;

private:
  std::string cfg_;
  uint8_t channel_{0};
  uint32_t sample_rate_{0};
  uint32_t sample_bit_{16};
};

} // namespace codec