#include "aac_frame.h"

namespace codec {

aac_frame::aac_frame(const network::flat_buffer::ptr &buf, uint64_t dts)
    : frame(CodecAAC) {
  frame::set_data(buf);
  frame::set_dts(dts);
  frame::set_pts(0);
  frame::set_prefix_size(0);
}

bool aac_frame::is_key_frame() const { return false; }

bool aac_frame::is_config_frame() const { return false; }

int aac_frame::get_frame_type() const { return -1; }

} // namespace codec