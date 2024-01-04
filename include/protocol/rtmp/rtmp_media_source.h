#pragma once

#include "media/media_source.h"
#include "rtmp_demuxer.h"

#include <mutex>
#include <unordered_map>

namespace rtmp {

class rtmp_media_source : public media::media_source
{
public:
    using ptr = std::shared_ptr<rtmp_media_source>;

    using Meta_Data = std::unordered_map<std::string, std::any>;

    using Config_Frame = std::unordered_map<int, rtmp_packet::ptr>;

    static ptr create(const media::media_info::ptr &);

    ~rtmp_media_source() = default;

    /// not responsible for releasing the Meta_Data *
    void init_tracks(Meta_Data *);

    void process_av_packet(rtmp_packet::ptr);

    Meta_Data *get_meta_data_or_fail();

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
    rtmp_media_source(const media::media_info::ptr &);

    // timestamps for audio/video
    uint32_t track_stamps_[2] = {0};
    rtmp_demuxer::ptr demuxer_;

    // meta data
    Meta_Data *meta_data_;

    mutable std::recursive_mutex config_mtx_;
    Config_Frame config_frame_map_{};
};

} // namespace rtmp