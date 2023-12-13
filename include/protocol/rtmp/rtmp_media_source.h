#pragma once

#include "media/media_source.h"

namespace rtmp {

class rtmp_media_source : public media::media_source {
public:
  using ptr = std::shared_ptr<rtmp_media_source>;

  static ptr create(const media::media_info::ptr &);

private:
  rtmp_media_source(const media::media_info::ptr &);
};

} // namespace rtmp