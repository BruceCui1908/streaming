#pragma once

#include "aac_frame.h"
#include "codec/track.h"

#include <array>

namespace codec {

static constexpr uint8_t kAdtsHeaderLength = 7;

// https://zhuanlan.zhihu.com/p/525616690?utm_id=0
static uint32_t samplingFrequencyTable[16] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0};

class adts_header
{
public:
    unsigned int syncword;                       // 12 bits, all bits must be 1
    unsigned int id;                             // 1 bit, MPEC version, 0 for MPEC-4, 1 for MPEC-2
    unsigned int layer;                          // 2 bits, always 0
    unsigned int protection_absent;              // 1 bit, 0 no crc, 1 has crc
    unsigned int profile;                        // 2 bits
    unsigned int sampling_frequency_index;       // 4 bits
    unsigned int private_bit;                    // 1 bit, 0 for encoding, 1 for decoding
    unsigned int channel_configuration;          // 3 bits
    unsigned int originality;                    // 1 bit, 0 for encoding, 1 for decoding
    unsigned int home;                           // 1 bit, 0 for encoding, 1 for decoding
    unsigned int copyright_identification_bit;   // 1 bit, 0 for encoding, 1 for decoding
    unsigned int copyright_identification_start; // 1 bit, 0 for encoding, 1 for decoding
    unsigned int aac_frame_length;               // 13 bits, protection_absent == 1 ? 7 : 9
    unsigned int adts_buffer_fullness;           // 11 bits, buffer fullness
    unsigned int no_raw_data_blocks_in_frame;    // 2 bits, number of AAC frames(RDBs) in ADTS
                                                 // frame minus 1, for maximum compatibility,
                                                 // always use 1 AAC frame per adts frame
                                                 /*unsigned int crc; 16 bits, crc if protection_absent is 0*/
};

class aac_track : public audio_track
{
public:
    using ptr = std::shared_ptr<aac_track>;

    uint32_t get_audio_sample_rate() override
    {
        return sample_rate_;
    }
    uint32_t get_audio_sample_bit() override
    {
        return sample_bit_;
    }
    uint8_t get_audio_channel() override
    {
        return channel_;
    }
    Codec_Type get_codec() override
    {
        return Codec_Type::CodecAAC;
    }

    void parse_config(const network::flat_buffer::ptr &) override;

    adts_header extract_aac_config();

    // need 7 bytes
    std::array<char, kAdtsHeaderLength> dump_adts_header(const adts_header &header);

private:
    std::string cfg_;
    uint8_t channel_{0};
    uint32_t sample_rate_{0};
    uint32_t sample_bit_{16};
};

} // namespace codec