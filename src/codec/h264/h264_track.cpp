#include "codec/h264/h264_track.h"

namespace codec {

int h264_track::get_video_height() const { return height_; }

int h264_track::get_video_width() const { return width_; }

double h264_track::get_video_fps() const { return fps_; }

Codec_Type h264_track::get_codec() { return Codec_Type::CodecH264; }
} // namespace codec