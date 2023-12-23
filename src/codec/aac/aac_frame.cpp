#include "aac_frame.h"

namespace codec {

aac_frame::aac_frame(const network::flat_buffer::ptr &buf, uint64_t dts)
    : frame(CodecAAC) {
  frame::set_data(buf);
  frame::set_dts(dts);
  frame::set_pts(0);
  frame::set_prefix_size(0);
}

} // namespace codec