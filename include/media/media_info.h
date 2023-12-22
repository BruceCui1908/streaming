#pragma once

#include "util/util.h"
#include <algorithm>
#include <cctype>
#include <memory>
#include <stdexcept>
#include <string>

#include <spdlog/spdlog.h>

namespace media {

class media_info : public std::enable_shared_from_this<media_info> {
public:
  using ptr = std::shared_ptr<media_info>;

  static constexpr char kVhostParam[] = "vhost=";
  static constexpr char kTokenParam[] = "token=";

  static ptr create(std::string schmea) {
    return std::shared_ptr<media_info>(new media_info(std::move(schmea)));
  }

  void set_schema(const std::string &schema = "RTMP") {
    if (schema.empty()) {
      throw std::invalid_argument("schema cannot be empty");
    }
    schema_ = schema;
  }

  const std::string &schema() const { return schema_; }

  void set_app(const std::string &app) {
    if (app.empty()) {
      throw std::invalid_argument("app cannot be empty");
    }
    app_ = app;
  }

  const std::string &app() const { return app_; }

  // parse stream like stream?vhost=test.com&token=abcdef
  void parse_stream(const std::string &stream) {
    static_assert(sizeof(kVhostParam) > 1, "kVhostParam is incorrect");
    static_assert(sizeof(kVhostParam) > 1, "kVhostParam is incorrect");

    if (stream.empty()) {
      throw std::invalid_argument("stream cannot be empty");
    }

    // find the first alphabet
    size_t start = 0;
    auto it = std::find_if(stream.begin(), stream.end(),
                           [](char c) { return std::isalpha(c); });
    if (it != stream.end()) {
      start = std::distance(stream.begin(), it);
    }

    auto stream_pos = stream.find_first_of('?', start);
    // if not ?, then the whole string is stream id
    if (stream_pos == std::string::npos) {
      stream_id_ = stream.substr(start);
    } else {
      stream_id_ = stream.substr(start, stream_pos - start);
    }

    auto vhost_pos = stream.find_first_of(kVhostParam, stream_pos);
    if (vhost_pos == std::string::npos) {
      return;
    }

    vhost_pos += sizeof(kVhostParam) - 1;

    auto amper_pos = stream.find_first_of('&', vhost_pos);
    if (amper_pos == std::string::npos) {
      vhost_ = stream.substr(vhost_pos);
    } else {
      vhost_ = stream.substr(vhost_pos, amper_pos - vhost_pos);
    }

    auto token_pos = stream.find_first_of(kTokenParam, amper_pos);
    if (token_pos == std::string::npos) {
      return;
    }

    token_pos += sizeof(kTokenParam) - 1;

    token_ = stream.substr(token_pos);
  }

  void set_stream_id(const std::string &stream_id) {
    if (stream_id.empty()) {
      throw std::invalid_argument("stream_id cannot be empty");
    }
    stream_id_ = stream_id;
  }

  const std::string &stream_id() const { return stream_id_; }

  void set_vhost(const std::string &vhost) {
    if (vhost.empty()) {
      throw std::invalid_argument("vhost cannot be empty");
    }
    vhost_ = vhost;
  }

  const std::string &vhost() const { return vhost_; }

  void set_token(const std::string &token) {
    if (token.empty()) {
      throw std::invalid_argument("token cannot be empty");
    }
    token_ = token;
  }

  const std::string &token() const { return token_; }

  void set_tc_url(const std::string &tc_url) { tc_url_ = tc_url; }

  const std::string &tc_url() const { return tc_url_; }

  const std::string &info() {
    if (info_.empty()) {
      info_ = fmt::format("{}|{}|{}|{}", schema_, app_, vhost_, stream_id_);
    }
    return info_;
  }

  bool is_complete() {
    return !schema_.empty() && !app_.empty() && !vhost_.empty() &&
           !stream_id_.empty();
  }

  ~media_info() { spdlog::debug("media_info {} destroyed", info_); }

private:
  media_info(std::string schmea) : schema_{schmea} {
    created_time_ = util::current_millis();
  }

private:
  std::string info_;

  std::string schema_;
  std::string app_;
  std::string vhost_;
  std::string stream_id_;
  std::string token_;
  std::string tc_url_;
  uint64_t created_time_{0};
};

} // namespace media
