#pragma once

#include "codec/frame.h"

#include <memory>

namespace media {

class muxer : public codec::frame_translator,
              public std::enable_shared_from_this<muxer> {
public:
  using ptr = std::shared_ptr<muxer>;

  static ptr create() { return std::shared_ptr<muxer>(new muxer); }

  void translate_frame(const codec::frame::ptr &) override;

private:
  muxer() = default;
};

} // namespace media
