#include "aac_track.h"

namespace codec {

// https://zhuanlan.zhihu.com/p/525616690?utm_id=0
static unsigned int samplingFrequencyTable[16] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,  7350,  0,     0,     0};

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

// need 7 bytes
static void dump_adts_header(const adts_header &header, uint8_t *out) {
  if (!out) {
    throw std::runtime_error("cannot dump to empty ");
  }

  // 1111 11111111   bit operation starts from the right end
  out[0] = (header.syncword >> 4 & 0xFF); // the upper 8 bits
  out[1] = (header.syncword << 4 & 0xF0); // the lower 4 bits
  out[1] |= (header.id << 3 & 0b1000);
  out[1] |= (header.layer << 1 & 0b0110); // 2 bits
  out[1] |= (header.protection_absent & 0b0001);

  out[2] = (header.profile << 6 & 0b11000000);                   // 2 bits
  out[2] |= (header.sampling_frequency_index << 2 & 0b00111100); // 4 bits
  out[2] |= (header.private_bit << 1 & 0b0010);                  // 1 bit
  out[2] |= (header.channel_configuration >> 2 & 0b0001);        // 1 bit

  out[3] = (header.channel_configuration << 6 & 0b11000000);           // 2 bits
  out[3] |= (header.originality << 5 & 0b00100000);                    // 1 bit
  out[3] |= (header.home << 4 & 0b00010000);                           // 1 bit
  out[3] |= (header.copyright_identification_bit << 3 & 0b00001000);   // 1 bit
  out[3] |= (header.copyright_identification_start << 2 & 0b00000100); // 1 bit
  out[3] |= (header.aac_frame_length >> 11 & 0b00000011);              // 2 bits

  out[4] = (header.aac_frame_length >> 3 & 0xFF); // 8 bits

  out[5] = (header.aac_frame_length << 5 & 0b11100000);      // 3 bits
  out[5] |= (header.adts_buffer_fullness >> 6 & 0b00011111); // 5 bits

  out[6] = (header.adts_buffer_fullness << 2 & 0b11111100);    // 6 bits
  out[6] |= (header.no_raw_data_blocks_in_frame & 0b00000011); // 2 bits
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
  int sample_index = ((cfg1 & 0b00000111) << 1) | (cfg2 >> 7);
  if (sample_index >= sizeof(samplingFrequencyTable)) {
    throw std::runtime_error(
        fmt::format("sample index {} is out of range", sample_index));
  }
  sample_rate_ = samplingFrequencyTable[sample_index];
  channel_ = (cfg2 & 0b01111000) >> 3;

  spdlog::debug("successfully parse aac config, sample_index = {}, samplerate "
                "= {}, channel = {}",
                sample_index, sample_rate_, channel_);
}

static frame::ptr prepend_aac_header(const frame::ptr &fr,
                                     const std::string &aac_conf) {
  if (!fr || aac_conf.empty()) {
    throw std::runtime_error("cannot prepend aac header with empty params");
  }

  const auto &ptr = fr->data();

  adts_header header;
  parse_aac_config(aac_conf, header);
  header.aac_frame_length = static_cast<decltype(header.aac_frame_length)>(
      ptr->unread_length() + kAdtsHeaderLength);

  char adts_head[7] = {0};
  dump_adts_header(header, (uint8_t *)adts_head);

  // create a new frame
  auto aac_ptr = network::flat_buffer::create();

  // append header to the frame
  aac_ptr->write(adts_head, sizeof(adts_head));
  // append aac to the frame
  aac_ptr->write(ptr->data(), ptr->unread_length());

  return std::make_shared<aac_frame>(aac_ptr, fr->dts());
}

/// all frames passed into this function do not have adts header
void aac_track::input_frame(const frame::ptr &fr) {

  // first p
  auto aac_complete_fr = prepend_aac_header(fr, cfg_);
}

} // namespace codec