#include "aac_track.h"

namespace codec {

void aac_track::parse_config(network::flat_buffer &buf)
{
    buf.require_length_or_fail(2);

    cfg_.assign(buf.data(), buf.unread_length());

    uint8_t cfg1 = cfg_[0];
    uint8_t cfg2 = cfg_[1];

    // the last 3 bits of cfg1 and the first bit of cfg2
    int sample_index = ((cfg1 & 0b00000111) << 1) | (cfg2 >> 7);
    if (sample_index >= std::size(samplingFrequencyTable))
    {
        throw std::runtime_error(fmt::format("aac sample index {} is out of range", sample_index));
    }
    sample_rate_ = samplingFrequencyTable[sample_index];
    channel_ = (cfg2 & 0b01111000) >> 3;

    spdlog::debug("successfully parsed aac config, sample_index = {}, samplerate "
                  "= {}, channel = {}",
        sample_index, sample_rate_, channel_);
}

adts_header aac_track::extract_aac_config()
{
    if (cfg_.empty() || cfg_.size() < 2)
    {
        throw std::runtime_error("aac config is not valid");
    }

    // aac sequence header is actually AudioSpecificConfiguration
    // 5 bits audioobjecttype
    // 4 bits sampling frequency index
    // 4 bits channel configuration
    // 1 bit framelengthflag
    // 1 bit dependsoncorecoder
    // 1 bit extention flag must be 0
    uint8_t cfg1 = cfg_[0];
    uint8_t cfg2 = cfg_[1];

    int audioObjectType;
    int sampling_fre_index;
    int channel_config;

    audioObjectType = cfg1 >> 3;                                   // the first 5 bits
    sampling_fre_index = ((cfg1 & 0b00000111) << 1) | (cfg2 >> 7); // the last 3 bits of cfg1 and the first bit of cfg2
    channel_config = (cfg2 & 0b01111000) >> 3;

    adts_header adts;
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

    return adts;
}

std::array<char, 7> aac_track::dump_adts_header(const adts_header &header)
{

    std::array<char, 7> out = {};

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

    return out;
}

} // namespace codec