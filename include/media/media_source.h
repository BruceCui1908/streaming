#pragma once

#include "media_info.h"

#include "codec/meta.h"
#include "util/timer.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>

namespace media {
#define MEDIA_SOURCE_PARAMS                                                    \
  const std::string &schema, const std::string &vhost, const std::string &app, \
      const std::string &stream_id

class media_source : public std::enable_shared_from_this<media_source> {
public:
  using ptr = std::shared_ptr<media_source>;

  /// bool denotes if the media source has been created*/
  static std::tuple<ptr, bool> find(MEDIA_SOURCE_PARAMS);

  std::shared_ptr<void> get_ownership();

  void regist();

  virtual ~media_source();
  media_source(const media_source &) = delete;
  media_source &operator=(const media_source &) = delete;
  media_source(media_source &&) = delete;
  media_source &operator=(media_source &&) = delete;

protected:
  media_source(const media_info::ptr &);

  int get_bytes_speed(codec::Track_Type);

private:
  void unregist();

protected:
  media_info::ptr media_info_;

  util::bytes_speed speed_[codec::Track_Type::Max];

private:
  std::atomic_flag owned_{false};
};
} // namespace media