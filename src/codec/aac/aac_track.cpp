#include "aac_track.h"

namespace codec {

class adts_header {
public:
  unsigned int syncword; // 12 bits, all bits must be 1
  unsigned int id;       // 1 bit, MPEC version, 0 for MPEC-4, 1 for MPEC-2
  unsigned int layer;    // 2 bits, always 0
  unsigned int protection_absent;        // 1 bit, 0 no crc, 1 has crc
  unsigned int profile;                  // 2 bits
  unsigned int sampling_frequency_index; // 4 bits
  unsigned int private_bit;           // 1 bit, 0 for encoding, 1 for decoding
  unsigned int channel_configuration; // 3 bits
  unsigned int originality;           // 1 bit, 0 for encoding, 1 for decoding
  unsigned int home;                  // 1 bit, 0 for encoding, 1 for decoding
  unsigned int
      copyright_identification_bit; // 1 bit, 0 for encoding, 1 for decoding
  unsigned int
      copyright_identification_start; // 1 bit, 0 for encoding, 1 for decoding
  unsigned int aac_frame_length;      // 13 bits, protection_absent == 1 ? 7 : 9
  unsigned int adts_buffer_fullness;  // 11 bits, buffer fullness
  unsigned int
      no_raw_data_blocks_in_frame; // 2 bits, number of AAC frames(RDBs) in ADTS
                                   // frame minus 1, for maximum compatibility,
                                   // always use 1 AAC frame per adts frame
  unsigned int crc;                // 16 bits, crc if protection_absent is 0
};

static void parse_aac_config(const std::string &config, adts_header &adts) {
  if (config.size() < 2) {
    throw std::runtime_error("aac config is not valid");
  }

  // aac sequence header is actually AudioSpecificConfiguration
  // 5 bits audioobjecttype
  // 4 bits sampling frequency index
  // 4 bits channel configuration
  // 1 bit framelengthflag
  // 1 bit dependsoncorecoder
  // 1 bit extention flag must be 0
  uint8_t cfg1 = config[0];
  uint8_t cfg2 = config[1];

  int audioObjectType;
  int sampling_fre_index;
  int channel_config;

  audioObjectType = cfg1 >> 3; // the first 5 bits
  sampling_fre_index =
      ((cfg1 & 0b00000111) << 1) |
      (cfg2 >> 7); // the last 3 bits of cfg1 and the first bit of cfg2
  channel_config = (cfg2 & 0b01111000) >> 3;

  adts.syncword = 0x0FFF;
  adts.id = 0;
  adts.layer = 0;
  adts.protection_absent = 1;
  adts.profile = audioObjectType - 1;
  adts.sampling_frequency_index = sampling_fre_index;
  adts.private_bit = 0;
  adts.channel_configuration = channel_config;
  adts.originality = 0;
  adts.home = 0;
  adts.copyright_identification_bit = 0;
  adts.copyright_identification_start = 0;
  adts.aac_frame_length = adts.protection_absent == 1 ? 7 : 9;
  adts.adts_buffer_fullness = 0b11111111111;
  adts.no_raw_data_blocks_in_frame = 0;
}

int aac_track::get_audio_sample_rate() { return sample_rate_; }

int aac_track::get_audio_sample_bit() { return sample_bit_; }

int aac_track::get_audio_channel() { return channel_; }

Codec_Type aac_track::get_codec() { return Codec_Type::CodecAAC; }

void aac_track::parse_config(const network::flat_buffer::ptr &buf) {
  if (!buf) {
    throw std::runtime_error("aac_track cannot parse empty config");
  }

  buf->require_length_or_fail(2);

  cfg_.assign(buf->data(), buf->unread_length());

  uint8_t cfg1 = cfg_[0];
  uint8_t cfg2 = cfg_[1];

  // the last 3 bits of cfg1 and the first bit of cfg2
  sample_rate_ = ((cfg1 & 0b00000111) << 1) | (cfg2 >> 7);
  channel_ = (cfg2 & 0b01111000) >> 3;

  spdlog::debug("successfully parse aac config, samplerate = {}, channel = {}",
                sample_rate_, channel_);
}

} // namespace codec