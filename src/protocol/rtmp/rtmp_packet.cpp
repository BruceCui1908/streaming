#include "rtmp_packet.h"

#include <array>
#include <stdexcept>

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

// restore everything except buf_ data
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

// https://www.jianshu.com/p/cc813ba41caa  based on the first byte which is
// FlvVideoTagHeader
bool rtmp_packet::is_video_keyframe() const {
  if (msg_type_id != MSG_VIDEO) {
    return false;
  }

  // enhanced-rtmp.pdf P7 parse flv video tagheader
  uint8_t flv_tag_header = buf_.peek_uint8();
  rtmp_av_frame_type frame_type;
  if ((flv_tag_header >> 4) & 0b1000) {
    // if the first bit is 1, then IsExHeader = true
    frame_type = (rtmp_av_frame_type)((flv_tag_header >> 4) & 0b0111);
  } else {
    // IsExHeader = false
    frame_type = (rtmp_av_frame_type)(flv_tag_header >> 4);
  }

  return frame_type == rtmp_av_frame_type::key_frame;
}

/// in RTMP flvtagheader, keyframe is config frame which contains sps/pps
bool rtmp_packet::is_config_frame() const {
  if (msg_type_id == MSG_VIDEO) {
    if (!is_video_keyframe()) {
      return false;
    }

    // if the frame is keyframe
    uint8_t flv_tag_header = buf_.peek_uint8();
    if ((flv_tag_header >> 4) & 0b1000) {
      // isExtHeader = true
      return (rtmp_av_ext_packet_type)(flv_tag_header & 0b00001111) ==
             rtmp_av_ext_packet_type::PacketTypeSequenceStart;
    }

    int codec_id = get_codec_id();
    if (!codec_id) {
      return false;
    }

    auto av_codec_id = (rtmp_video_codec)(codec_id);
    if (av_codec_id == rtmp_video_codec::h264 ||
        av_codec_id == rtmp_video_codec::h265) {
      if (buf_.unread_length() < 2) {
        throw std::runtime_error(
            "not enough data to parse rtmp video tag header");
      }

      // check if the frame is sps/pps
      return (rtmp_h264_packet_type)(buf_.data()[1]) ==
             rtmp_h264_packet_type::h264_config_header;
    }

    return false;
  }

  if (msg_type_id == MSG_AUDIO) {
    buf_.ensure_length(2);
    int codec_id = get_codec_id();

    return (rtmp_audio_codec)(codec_id) == rtmp_audio_codec::aac &&
           (rtmp_aac_packet_type)(buf_.data()[1]) ==
               rtmp_aac_packet_type::aac_config_header;
  }

  return false;
}

int rtmp_packet::get_codec_id() const {
  uint8_t flv_tag_header = buf_.peek_uint8();
  switch (msg_type_id) {
  case MSG_VIDEO:
    // use lower 4 bits
    return (uint8_t)(flv_tag_header & 0x0F);
  case MSG_AUDIO:
    // use higher 4 bits
    return (uint8_t)(flv_tag_header >> 4);
  default:
    return -1;
  }
}

void rtmp_packet::set_header_len(size_t header_len) {
  if (header_len) {
    header_length_ += header_len;
  }
}

size_t rtmp_packet::size() { return header_length_ += buf_.unread_length(); }

} // namespace rtmp