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
    : media_source(info) {
  demuxer_ = std::make_shared<rtmp_demuxer>();
}

void rtmp_media_source::init_tracks(
    std::unordered_map<std::string, std::any> &meta_data) {
  demuxer_->init_tracks_with_metadata(meta_data);
}

// process rtmp audio/video packet
void rtmp_media_source::process_av_packet(rtmp_packet::ptr pkt) {
  // using
  demuxer_->input_rtmp(pkt);

  // if (!pkt || pkt->msg_type_id != MSG_AUDIO || pkt->msg_type_id != MSG_VIDEO)
  // {
  //   throw std::runtime_error("rtmp packet must be either video or audio");
  // }

  // // update timestamp for audio/video tracks
  // Track_Type type =
  //     pkt->msg_type_id == MSG_AUDIO ? Track_Type::Audio : Track_Type::Video;
  // track_stamps_[type] = pkt->time_stamp;

  // TODO
}

} // namespace rtmp