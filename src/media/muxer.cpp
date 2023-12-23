#include "muxer.h"

#include <spdlog/spdlog.h>

namespace media {

void muxer::translate_frame(const codec::frame::ptr &fr) {
  // all protocol translation are implemented here
  if (!fr) {
    return;
  }

  // TODO
}

} // namespace media