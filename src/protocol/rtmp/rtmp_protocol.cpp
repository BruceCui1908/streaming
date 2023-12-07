#include "rtmp_protocol.h"

#include <spdlog/spdlog.h>

namespace rtmp {

static constexpr size_t kMaxCacheSize = 4 * 1024 * 1024;

rtmp_protocol::rtmp_protocol() {
  // initialize buffer
  pool_ = util::resource_pool<network::buffer_raw>::create();
  next_step_func_ = [this](const char *data, size_t size) {
    return handle_C0C1(data, size);
  };
}

void rtmp_protocol::on_parse_rtmp(const char *data, size_t size) {
  if (cache_data_.unread_length() > kMaxCacheSize) {
    throw std::out_of_range(
        "cached rtmp protocol data length is out of range!");
  }

  const char *dummy = data;
  if (cache_data_.unread_length() > 0) {
    cache_data_.write(data, size);
    data = dummy = cache_data_.data();
    size = cache_data_.unread_length();
  }

  if (!next_step_func_) {
    throw std::runtime_error("rtmp next_step_func is empty!");
  }

  const char *index = nullptr;
  do {
    index = next_step_func_(data, size);
  } while (index != nullptr);
}

const char *rtmp_protocol::handle_C0C1(const char *data, size_t size) {
  // TODO
  return nullptr;
}

} // namespace rtmp