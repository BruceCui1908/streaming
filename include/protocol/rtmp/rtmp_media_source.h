#pragma once

#include "media/media_source.h"
#include "rtmp_demuxer.h"
#include "media/packet_dispatcher.h"

#include <unordered_map>

namespace rtmp {

class rtmp_media_source : public media::media_source
{
public:
    using ptr = std::shared_ptr<rtmp_media_source>;

    using metadata_map = std::unordered_map<std::string, std::any>;
    using config_frame_map = std::unordered_map<int, rtmp_packet::ptr>;
    using rtmp_dispatcher = media::packet_dispatcher<rtmp_packet::ptr>;

    static ptr create(media::media_info::ptr);

    ~rtmp_media_source() = default;

    /// not responsible for releasing the metadata_map *
    void init_tracks(metadata_map *);

    void process_av_packet(rtmp_packet::ptr);

    metadata_map *get_metadata();

    rtmp_dispatcher::ptr get_dispatcher();

    template<typename FUNC>
    void loop_config_frame(const FUNC &func)
    {
        std::lock_guard<std::recursive_mutex> lock(config_mtx_);
        for (auto &pr : config_frame_map_)
        {
            func(pr.second);
        }
    }

private:
    rtmp_media_source(media::media_info::ptr);

    // timestamps for audio/video
    uint32_t track_stamps_[2] = {0};
    rtmp_demuxer::ptr demuxer_;

    // meta data
    metadata_map *meta_data_;

    mutable std::recursive_mutex config_mtx_;
    config_frame_map config_frame_map_{};

    // packet dispatcher
    rtmp_dispatcher::ptr dispatcher_;
};

} // namespace rtmp