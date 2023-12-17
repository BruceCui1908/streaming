#pragma once

namespace codec {
typedef enum {
  CodecInvalid = -1,
  CodecH264 = 0,
  CodecAAC = 1,
} Codec_Type;

typedef enum {
  Audio = 0,
  Video = 1,
  Max,
} Track_Type;
} // namespace codec