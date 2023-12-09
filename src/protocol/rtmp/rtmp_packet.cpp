#include "rtmp_packet.h"

#include <array>

namespace rtmp {
// rtmp_handshake
rtmp_handshake::rtmp_handshake() {
  static char cdata[] = {
      0x73, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x2d, 0x72, 0x74, 0x6d, 0x70, 0x2d,
      0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x2d, 0x77, 0x69, 0x6e, 0x6c, 0x69,
      0x6e, 0x2d, 0x77, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x73, 0x65, 0x72, 0x76,
      0x65, 0x72, 0x40, 0x31, 0x32, 0x36, 0x2e, 0x63, 0x6f, 0x6d};

  for (auto i = 0; i < RANDOM_HANDSHAKE_SIZE; ++i) {
    random_[i] = cdata[std::rand() % (sizeof(cdata) - 1)];
  }
}

// rtmp_packet
rtmp_packet::ptr rtmp_packet::create() {
  return std::shared_ptr<rtmp_packet>(new rtmp_packet);
}

// restore everything except buffer data
rtmp_packet &rtmp_packet::restore_context(const rtmp_packet &other) {
  is_abs_stamp = other.is_abs_stamp;
  chunk_stream_id = other.chunk_stream_id;
  msg_stream_id = other.msg_stream_id;
  msg_length = other.msg_length;
  msg_type_id = other.msg_type_id;
  ts_delta = other.ts_delta;
  time_stamp = other.time_stamp;
  return *this;
}

} // namespace rtmp