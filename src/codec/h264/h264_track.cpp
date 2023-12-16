#include "codec/h264/h264_track.h"
#include "SPSParser.h"
#include <cstring>

#include <spdlog/spdlog.h>

namespace codec {

int h264_track::get_video_height() const { return height_; }

int h264_track::get_video_width() const { return width_; }

double h264_track::get_video_fps() const { return fps_; }

Codec_Type h264_track::get_codec() { return Codec_Type::CodecH264; }

// https://www.jianshu.com/p/4f95617f30d0
void h264_track::parse_config(network::flat_buffer &buf) {
  // byte[0] version
  // byte[1] avc profile
  // byte[2] avc compatibility
  // byte[3] avc level
  // byte[4] FF
  // byte[5] E1
  if (buf.unread_length() < 6) {
    throw std::runtime_error("not enough data to parse sps/pps");
  }
  buf.consume(6);

  // byte[6] byte[7] sps length
  auto sps_size = buf.read_uint16();
  if (buf.unread_length() < sps_size) {
    throw std::runtime_error("not enough data to parse body of sps");
  }

  sps_.assign(buf.data(), sps_size);
  buf.consume(sps_size);

  // skip the start of
  if (!buf.unread_length()) {
    throw std::runtime_error("no data to parse pps");
  }
  buf.consume(1);

  auto pps_size = buf.read_uint16();
  if (buf.unread_length() < pps_size) {
    throw std::runtime_error("not enough data to parse body of pps");
  }
  pps_.assign(buf.data(), pps_size);
  buf.consume(pps_size);

  // parse width height fps
  if (sps_.size() < 4) {
    throw std::runtime_error("not enough data to parse sps params");
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
      "successfully parsed sps/pps, video width = {}, height = {}, fps = {}",
      width_, height_, fps_);
}

} // namespace codec