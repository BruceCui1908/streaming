#pragma once

#include "media_info.h"

#include "codec/meta.h"
#include "util/timer.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <tuple>
#include <unordered_map>

namespace media {
#define MEDIA_SOURCE_PARAMS const std::string &schema, const std::string &vhost, const std::string &app, const std::string &stream_id

class media_source : public std::enable_shared_from_this<media_source>
{
public:
    using ptr = std::shared_ptr<media_source>;

    virtual ~media_source();

    /// bool denotes if the media source has been created*/
    static std::tuple<ptr, bool> find(MEDIA_SOURCE_PARAMS);

    std::shared_ptr<void> get_ownership();

    bool is_registered();

    media_source(const media_source &) = delete;
    media_source &operator=(const media_source &) = delete;
    media_source(media_source &&) = delete;
    media_source &operator=(media_source &&) = delete;

protected:
    media_source(media_info::ptr);

    void set_registered(bool);
    void regist();
    void unregist();

    int get_bytes_speed(codec::Track_Type);

protected:
    media_info::ptr media_info_;

    util::bytes_speed speed_[codec::kTrackCount];

private:
    std::atomic_flag owned_{false};
    std::atomic_bool is_registered_{false};
};
} // namespace media