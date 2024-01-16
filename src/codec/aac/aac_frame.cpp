#include "aac_frame.h"

namespace codec {

aac_frame::aac_frame(network::flat_buffer::ptr buf, uint64_t dts)
    : frame(codec::Codec_Type::CodecAAC)
{
    frame::set_data(std::move(buf));
    frame::set_dts(dts);
    frame::set_pts(0);
    frame::set_prefix_size(0);
}

} // namespace codec