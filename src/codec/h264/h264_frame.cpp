#include "h264_frame.h"

namespace codec {

h264_frame::h264_frame()
    : frame(Codec_Type::CodecH264)
{}

h264_frame::Nal_Type h264_frame::frame_type() const
{
    const auto &ptr = data();
    ptr->require_length_or_fail(frame::prefix_size());
    uint8_t *nal_ptr = (uint8_t *)ptr->data() + frame::prefix_size();
    return Nal_Type(nal_ptr[0] & 0x1F);
}

bool h264_frame::is_key_frame() const
{
    return frame_type() == Nal_Type::NAL_IDR;
}

bool h264_frame::is_config_frame() const
{
    auto ft = frame_type();
    return ft == Nal_Type::NAL_SPS || ft == Nal_Type::NAL_PPS;
}

} // namespace codec