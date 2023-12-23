#pragma once

#include "codec/frame.h"

namespace codec {

class aac_frame : public codec::frame {
public:
  using ptr = std::shared_ptr<aac_frame>;

  aac_frame(const network::flat_buffer::ptr &, uint64_t dts);

  bool is_key_frame() const override { return false; }
  bool is_config_frame() const override { return false; }
};

} // namespace codec