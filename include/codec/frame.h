#pragma once

#include "meta.h"
#include "network/flat_buffer.h"
#include <memory>
#include <stdint.h>

namespace codec {

class frame {
public:
  using ptr = std::shared_ptr<frame>;

  frame() = default;
  virtual ~frame() = default;

  uint32_t dts() const { return dts_; }
  void set_dts(uint32_t ds) { dts_ = ds; }

  uint32_t pts() const { return pts_ ? pts_ : dts_; }
  void set_pts(uint32_t ps) { pts_ = ps; }

  // h264 is 4, aac is 7
  void set_prefix_size(uint8_t size) { prefix_size_ = size; }
  size_t prefix_size() const { return prefix_size_; }

  void set_codec_id(Codec_Type codec_id) { codec_id_ = codec_id; }
  Codec_Type get_codec_id() const { return codec_id_; }

  void set_data(const network::flat_buffer::ptr &ptr) { buf_ = ptr; }
  const network::flat_buffer::ptr &data() const { return buf_; }

  virtual bool is_key_frame() const = 0;
  virtual bool is_config_frame() const = 0;
  virtual int get_frame_type() const = 0;

private:
  Codec_Type codec_id_{Codec_Type::CodecInvalid};
  uint32_t dts_{0};
  uint32_t pts_{0};
  uint8_t prefix_size_{0};
  network::flat_buffer::ptr buf_;
};

} // namespace codec