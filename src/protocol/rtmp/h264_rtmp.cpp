#include "h264_rtmp.h"
#include "codec/h264/h264_track.h"

namespace rtmp {
void h264_rtmp_decoder::input_rtmp(const rtmp_packet::ptr &pkt) {
  // if the frame is sps/pps
  if (pkt->is_config_frame()) {
    // tag header(1 byte) + packet type(1 byte) + composition time(3 bytes)
    if (pkt->buffer.unread_length() <= 5) {
      throw std::runtime_error("not enough data to process h264 config frame");
    }

    pkt->buffer.consume(5);

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

    h264_track_ptr->parse_config(pkt->buffer);
    return;
  }

  // if not pps/sps

  // TODO
}
} // namespace rtmp