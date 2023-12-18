#pragma once

#include "muxer.h"

namespace media {

class demuxer {
public:
  demuxer() = default;
  virtual ~demuxer() = default;

  void set_muxer(const muxer::ptr &mux) { muxer_ = mux; }

  const muxer::ptr &get_muxer() { return muxer_; }

private:
  muxer::ptr muxer_;
};

} // namespace media