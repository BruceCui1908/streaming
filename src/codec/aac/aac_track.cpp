#include "aac_track.h"

namespace codec {
int aac_track::get_audio_sample_rate() { return sample_rate_; }

int aac_track::get_audio_sample_bit() { return sample_bit_; }

int aac_track::get_audio_channel() { return channel_; }

Codec_Type aac_track::get_codec() { return Codec_Type::CodecAAC; }

} // namespace codec