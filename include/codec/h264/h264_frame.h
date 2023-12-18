#pragma once

#include "codec/frame.h"

namespace codec {

class h264_frame : public codec::frame {
public:
  using ptr = std::shared_ptr<h264_frame>;

  // https://zhuanlan.zhihu.com/p/622152133  nal_unit_type
  enum {
    NAL_S_P = 1, // coded sliced partition
    NAL_IDR = 5,
    NAL_SEI = 6,
    NAL_SPS = 7,
    NAL_PPS = 8,
    NAL_AUD = 9,
  };

  static ptr create() { return std::make_shared<h264_frame>(); }

  h264_frame();

  int get_frame_type() const override;
  bool is_key_frame() const override;
  bool is_config_frame() const override;
};
} // namespace codec