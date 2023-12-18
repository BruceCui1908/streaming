#include "h264_frame.h"

namespace codec {

h264_frame::h264_frame() : frame(Codec_Type::CodecH264) {}

int h264_frame::get_frame_type() const {
  if (!data() || data()->unread_length() < frame::prefix_size()) {
    throw std::runtime_error("h264 frame does not contain prefix");
  }
  uint8_t *nal_ptr = (uint8_t *)data()->data() + frame::prefix_size();
  return nal_ptr[0] & 0x1F;
}

bool h264_frame::is_key_frame() const { return get_frame_type() == NAL_IDR; }

bool h264_frame::is_config_frame() const {
  int ft = get_frame_type();
  return ft == NAL_SPS || ft == NAL_PPS;
}

} // namespace codec