#include "codec/h264/h264_track.h"
#include "SPSParser.h"

namespace codec {

int h264_track::get_video_height() const { return height_; }

int h264_track::get_video_width() const { return width_; }

float h264_track::get_video_fps() const { return fps_; }

Codec_Type h264_track::get_codec() { return Codec_Type::CodecH264; }

void h264_track::extract_bitstream_sps() {
  if (sps_.empty() || sps_.size() < 4) {
    spdlog::error("sps {} is not valid", sps_);
    return;
  }

  T_GetBitContext tGetBitBuf;
  T_SPS tH264SpsInfo;
  std::memset(&tGetBitBuf, 0, sizeof(tGetBitBuf));
  std::memset(&tH264SpsInfo, 0, sizeof(tH264SpsInfo));

  tGetBitBuf.pu8Buf = (uint8_t *)sps_.data() + 1;
  tGetBitBuf.iBufSize = (int)(sps_.size() - 1);
  if (0 != h264DecSeqParameterSet((void *)&tGetBitBuf, &tH264SpsInfo)) {
    throw std::runtime_error("cannot parse sps");
  }

  h264GetWidthHeight(&tH264SpsInfo, &width_, &height_);
  h264GeFramerate(&tH264SpsInfo, &fps_);

  spdlog::debug(
      "successfully parsed sps, video width = {}, height = {}, fps = {}",
      width_, height_, fps_);
}

// https://www.jianshu.com/p/4f95617f30d0
void h264_track::parse_config(const network::flat_buffer::ptr &buf) {
  if (!buf) {
    throw std::runtime_error("h264_track cannot parse empty config");
  }

  // byte[0] version
  // byte[1] avc profile
  // byte[2] avc compatibility
  // byte[3] avc level
  // byte[4] FF
  // byte[5] E1
  buf->consume_or_fail(6);

  // byte[6] byte[7] sps length
  auto sps_size = buf->read_uint16();
  buf->require_length_or_fail(sps_size);

  sps_.assign(buf->data(), sps_size);
  buf->consume_or_fail(sps_size);

  // skip the byte 01
  buf->consume_or_fail(1);

  auto pps_size = buf->read_uint16();
  buf->require_length_or_fail(pps_size);

  pps_.assign(buf->data(), pps_size);
  buf->consume_or_fail(pps_size);

  extract_bitstream_sps();

  // pass sps frame to muxer
  encapsulate_config_frame(sps_);

  // pass pps frame to muxer
  encapsulate_config_frame(pps_);
}

void h264_track::encapsulate_config_frame(const std::string &config) {
  if (config.empty()) {
    return;
  }

  auto config_ptr = network::flat_buffer::create();
  config_ptr->write("\x00\x00\x00\x01", 4);
  config_ptr->write(config.data(), config.size());

  auto config_frame = h264_frame::create();
  config_frame->set_prefix_size(4);
  config_frame->set_dts(0);
  config_frame->set_pts(0);
  config_frame->set_data(config_ptr);
  track::input_frame(config_frame);
}

} // namespace codec