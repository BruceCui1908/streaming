#include "h264_rtmp.h"

namespace rtmp {
void h264_rtmp_decoder::input_rtmp(const rtmp_packet::ptr &pkt) {
  // if the frame is sps/pps
  if (pkt->is_config_frame()) {
    // tag header(1 byte) + packet type(1 byte) + composition time(3 bytes)
    if (pkt->buffer.unread_length() <= 5) {
      throw std::runtime_error("not enough data to process h264 config frame");
    }

    // TODO parse sps pps
  }

  // TODO
}
} // namespace rtmp