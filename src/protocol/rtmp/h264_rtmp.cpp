#include "h264_rtmp.h"
#include "codec/h264/h264_frame.h"
#include "codec/h264/h264_track.h"

namespace rtmp {

/// @brief If the packet is a configuration frame, it is then passed to the
/// H.264 track to extract the SPS/PPS. Otherwise, it is split and then passed
/// to the H.264 track for further processing
/// @param pkt
void h264_rtmp_decoder::input_rtmp(rtmp_packet::ptr pkt)
{
    if (!pkt)
    {
        return;
    }

    // tag header(1 byte) + packet type(1 byte) + composition time(3 bytes)
    const auto &buf = pkt->buf();
    buf->require_length_or_fail(5);

    // if the frame is sps/pps
    if (pkt->is_config_frame())
    {
        buf->consume_or_fail(5);
        const auto track_ptr = get_track();
        if (!track_ptr)
        {
            throw std::runtime_error("The video track must be attached to the "
                                     "h264_rtmp_decoder prior to processing RTMP");
        }

        auto h264_track_ptr = std::dynamic_pointer_cast<codec::h264_track>(track_ptr);
        if (!h264_track_ptr)
        {
            throw std::runtime_error("The video track cannot be cast to h264 track in h264_rtmp_decoder");
        }

        h264_track_ptr->parse_config(*buf);
        return;
    }

    // if not pps/sps, skip tag header(1 byte) and packet type(1 byte)
    buf->consume_or_fail(2);

    // https://github.com/FFmpeg/FFmpeg/blob/master/libavformat/flvdec.c 1293
    uint8_t *cts_ptr = reinterpret_cast<uint8_t *>(buf->data());
    int32_t cts = (((cts_ptr[0] << 16) | (cts_ptr[1] << 8) | (cts_ptr[2])) + 0xff800000) ^ 0xff800000;
    uint32_t pts = pkt->time_stamp + cts;
    buf->consume_or_fail(3);
    split_nal_frame(buf, pkt->time_stamp, pts);
}

/// @brief split rtmp frame by frame length, not prefix
void h264_rtmp_decoder::split_nal_frame(const network::flat_buffer::ptr &buf, uint32_t dts, uint32_t pts)
{
    static_assert(sizeof(codec::kH264HeaderPrefix) == 5);

    const auto track_ptr = get_track();

    while (buf->unread_length() >= 4)
    {
        auto frame_len = buf->read_uint32();
        if (frame_len > buf->unread_length())
        {
            break;
        }

        auto h264_frame_len = sizeof(codec::kH264HeaderPrefix) - 1 + frame_len;
        auto ptr = network::flat_buffer::create(h264_frame_len);
        ptr->write(codec::kH264HeaderPrefix, sizeof(codec::kH264HeaderPrefix) - 1);
        ptr->write(buf->data(), frame_len);
        buf->consume_or_fail(frame_len);

        codec::h264_frame hr;
        hr.set_prefix_size(4);
        hr.set_dts(dts);
        hr.set_pts(pts);
        hr.set_data(ptr);

        track_ptr->input_frame(hr);
    }

    if (buf->unread_length())
    {
        spdlog::error("Error while splitting H.264 frames\n {}", buf->dump());
        throw std::runtime_error("H.264 frames are encoded incorrectly");
    }
}

} // namespace rtmp