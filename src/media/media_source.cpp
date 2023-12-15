#include "media_source.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace media {
// store all the media sources
static std::recursive_mutex media_sources_mtx_;
using stream_map =
    std::unordered_map<std::string /**stream id*/, std::weak_ptr<media_source>>;
using app_map = std::unordered_map<std::string /**app*/, stream_map>;
using vhost_map = std::unordered_map<std::string /**vhost*/, app_map>;
using schema_map = std::unordered_map<std::string /**schema*/, vhost_map>;

static schema_map media_sources_{};

media_source::media_source(const media_info::ptr &ptr) {
  if (!ptr) {
    throw std::invalid_argument(
        "cannot build media_source with empty media_info");
  }

  if (!ptr->is_complete()) {
    throw std::invalid_argument(
        "cannot build media_source with incomplete media_info");
  }

  media_info_ = ptr;
}

media_source::~media_source() { unregist(); }

void media_source::regist() {
  if (!media_info_) {
    throw std::runtime_error("cannot regist source with empty media info");
  }

  std::lock_guard<std::recursive_mutex> lock(media_sources_mtx_);
  auto &weak_source =
      media_sources_[media_info_->schema()][media_info_->vhost()]
                    [media_info_->app()][media_info_->stream_id()];
  auto strong_source = weak_source.lock();
  if (strong_source) {
    if (strong_source.get() == this) {
      spdlog::warn("media source {} has already been registered",
                   media_info_->info());
      return;
    }

    throw std::runtime_error(fmt::format(
        "another client is trying to push source to {}", media_info_->info()));
  }

  weak_source = shared_from_this();
}

void media_source::unregist() {
  // TODO
}

std::tuple<media_source::ptr, bool>
media_source::find(const std::string &schema, const std::string &vhost,
                   const std::string &app, const std::string &stream_id) {
  if (schema.empty() || vhost.empty() || app.empty() || stream_id.empty()) {
    throw std::invalid_argument("cannot find media source with empty params");
  }

  std::lock_guard<std::recursive_mutex> lock(media_sources_mtx_);
  // check if schema exists in media sources
  if (!media_sources_.count(schema)) {
    spdlog::debug("cannot find media source with {}", schema);
    return {nullptr, false};
  }

  auto &vhost_sources = media_sources_[schema];
  if (!vhost_sources.count(vhost)) {
    spdlog::debug("cannot find media source with {}|{}", schema, vhost);
    return {nullptr, false};
  }

  auto &app_sources = vhost_sources[vhost];
  if (!app_sources.count(app)) {
    spdlog::debug("cannot find media source with {}|{}", app);
    return {nullptr, false};
  }

  auto &stream_sources = app_sources[app];
  if (!stream_sources.count(stream_id)) {
    spdlog::debug("media sources do not contain stream id {}", stream_id);
    return {nullptr, false};
  }

  return {stream_sources[stream_id].lock(), true};
}

std::shared_ptr<void> media_source::get_ownership() {
  // the source has already been owned
  if (owned_.test_and_set()) {
    return nullptr;
  }

  std::weak_ptr<media_source> weak_self = shared_from_this();
  return std::shared_ptr<void>((void *)(0x01), [weak_self](void *ptr) {
    auto strong_self = weak_self.lock();
    if (strong_self) {
      strong_self->owned_.clear();
    }
  });
}

} // namespace media