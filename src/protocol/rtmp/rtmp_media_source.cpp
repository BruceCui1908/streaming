#include "rtmp_media_source.h"

namespace rtmp {

rtmp_media_source::ptr rtmp_media_source::create(const media::media_info::ptr &info)
{
    if (!info || info->schema() != "RTMP")
    {
        throw std::invalid_argument("media_info is invalid");
    }
    return std::shared_ptr<rtmp_media_source>(new rtmp_media_source(info));
}

rtmp_media_source::rtmp_media_source(const media::media_info::ptr &info)
    : media_source(info)
{
    demuxer_ = std::make_shared<rtmp_demuxer>();
    demuxer_->set_muxer(media::muxer::create());
}

void rtmp_media_source::init_tracks(std::unordered_map<std::string, std::any> *meta_data)
{
    demuxer_->init_tracks_with_metadata(meta_data);
    meta_data_ = meta_data;
    // init dispatcher after tracks have been initialized
    dispatcher_ = rtmp_dispatcher::create();
}

rtmp_media_source::metadata_map *rtmp_media_source::get_metadata()
{
    if (!meta_data_)
    {
        throw std::runtime_error("meta data is empty");
    }
    return meta_data_;
}

rtmp_media_source::rtmp_dispatcher::ptr rtmp_media_source::get_dispatcher()
{
    if (!dispatcher_)
    {
        throw std::runtime_error("rtmp dispatcher is empty");
    }

    return dispatcher_;
}

// process rtmp audio/video packet
void rtmp_media_source::process_av_packet(rtmp_packet::ptr pkt)
{
    if (!pkt || (pkt->msg_type_id != MSG_AUDIO && pkt->msg_type_id != MSG_VIDEO))
    {
        throw std::runtime_error("rtmp packet must be either video or audio");
    }

    if (!dispatcher_)
    {
        throw std::runtime_error("Must initialize tracks with metadata before processing packets.");
    }

    // parsed by demuxer
    demuxer_->input_rtmp(pkt);

    // update speed
    bool is_video = pkt->msg_type_id == MSG_VIDEO;
    speed_[is_video ? codec::Track_Type::Video : codec::Track_Type::Audio] += pkt->size();

    // update timestamp for audio/video tracks
    track_stamps_[is_video ? codec::Track_Type::Video : codec::Track_Type::Audio] = pkt->time_stamp;

    // always update the up-to-date config frame for both video and audio
    if (pkt->is_config_frame())
    {
        {
            std::scoped_lock lock(config_mtx_);
            config_frame_map_[pkt->msg_type_id] = pkt;
        }
    }

    // TODO add packet dispatcher
}

} // namespace rtmp