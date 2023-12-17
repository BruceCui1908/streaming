#include "h264_rtmp.h"
#include "codec/h264/h264_frame.h"
#include "codec/h264/h264_track.h"

namespace rtmp {

/// @brief If the packet is a configuration frame, it is then passed to the
/// H.264 track to extract the SPS/PPS. Otherwise, it is split and then passed
/// to the H.264 track for further processing
/// @param pkt
void h264_rtmp_decoder::input_rtmp(rtmp_packet::ptr &pkt) {

  // tag header(1 byte) + packet type(1 byte) + composition time(3 bytes)
  pkt->buf_.ensure_length(5);
  pkt->buf_.capture_snapshot();

  // if the frame is sps/pps
  if (pkt->is_config_frame()) {
    pkt->buf_.consume(5);
    const auto track_ptr = get_track();
    if (!track_ptr) {
      throw std::runtime_error("The video track must be attached to the "
                               "h264_rtmp_decoder prior to processing RTMP");
    }

    auto h264_track_ptr =
        std::dynamic_pointer_cast<codec::h264_track>(track_ptr);
    if (!h264_track_ptr) {
      throw std::runtime_error(
          "The video track cannot be cast to h264 track in h264_rtmp_decoder");
    }

    h264_track_ptr->parse_config(pkt->buf_);
    pkt->buf_.restore_snapshot();
    return;
  }

  // if not pps/sps, skip tag header(1 byte) and packet type(1 byte)
  pkt->buf_.consume(2);

  // https://github.com/FFmpeg/FFmpeg/blob/master/libavformat/flvdec.c 1293
  uint8_t *cts_ptr = (uint8_t *)(pkt->buf_.data());
  int32_t cts =
      (((cts_ptr[0] << 16) | (cts_ptr[1] << 8) | (cts_ptr[2])) + 0xff800000) ^
      0xff800000;
  uint32_t pts = pkt->time_stamp + cts;
  pkt->buf_.consume(3);
  split_nal_frame(pkt->buf_, pkt->time_stamp, pts);
}

/// @brief split rtmp frame by frame length, not prefix
void h264_rtmp_decoder::split_nal_frame(network::flat_buffer &buf, uint32_t dts,
                                        uint32_t pts) {
  const auto track_ptr = get_track();

  while (buf.unread_length() >= 4) {
    auto frame_len = buf.read_uint32();
    if (frame_len > buf.unread_length()) {
      break;
    }

    auto ptr = network::flat_buffer::create();
    ptr->write("\x00\x00\x00\x01", 4);
    ptr->write(buf.data(), frame_len);
    buf.consume(frame_len);

    auto frame_ptr = codec::h264_frame::create();
    frame_ptr->set_prefix_size(4);
    frame_ptr->set_dts(dts);
    frame_ptr->set_pts(pts);
    frame_ptr->set_data(ptr);

    track_ptr->input_frame(frame_ptr);
  }

  spdlog::debug(
      "h264_rtmp_decoder finished splitting frames, the buf remain size is {}",
      buf.unread_length());
}

} // namespace rtmp