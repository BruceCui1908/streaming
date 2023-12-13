#include "rtmp_media_source.h"

namespace rtmp {

rtmp_media_source::ptr
rtmp_media_source::create(const media::media_info::ptr &info) {
  if (!info || info->schema() != "RTMP") {
    throw std::invalid_argument("media_info is invalid");
  }
  return std::shared_ptr<rtmp_media_source>(new rtmp_media_source(info));
}

rtmp_media_source::rtmp_media_source(const media::media_info::ptr &info)
    : media_source(info) {}

} // namespace rtmp