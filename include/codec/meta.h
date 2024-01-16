#pragma once

#include "util/magic_enum.hpp"

namespace codec {

enum class Codec_Type
{
    CodecInvalid = -1,
    CodecH264 = 0,
    CodecAAC = 1,
};

enum class Track_Type
{
    Audio = 0,
    Video = 1,
};

constexpr std::size_t kTrackCount = magic_enum::enum_count<Track_Type>();

} // namespace codec