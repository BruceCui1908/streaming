#include "rtmp_demuxer.h"

#include "codec/aac/aac_track.h"
#include "codec/h264/h264_track.h"
#include <stdexcept>

namespace rtmp {

/// init video/audio tracks based on the metadata passed by the client, eg obs
void rtmp_demuxer::init_tracks_with_metadata(
    std::unordered_map<std::string, std::any> &metadata) {
  if (metadata.empty()) {
    throw std::runtime_error("cannot init tracks with empty meta data");
  }

  duration_ = std::any_cast<double>(metadata["duration"]);

  int audiosamplerate =
      static_cast<int>(std::any_cast<double>(metadata["audiosamplerate"]));

  int audiosamplesize =
      static_cast<int>(std::any_cast<double>(metadata["audiosamplesize"]));

  int audiochannels = std::any_cast<bool>(metadata["stereo"]) ? 2 : 1;

  int videocodecid =
      static_cast<int>(std::any_cast<double>(metadata["videocodecid"]));

  int audiocodecid =
      static_cast<int>(std::any_cast<double>(metadata["audiocodecid"]));

  int audiodatarate =
      static_cast<int>(std::any_cast<double>(metadata["audiodatarate"]));

  int videodatarate =
      static_cast<int>(std::any_cast<double>(metadata["videodatarate"]));

  // has video track
  if (videocodecid) {
    init_video_track(videocodecid, videodatarate * 1024);
  }

  if (audiocodecid) {
    init_audio_track(audiocodecid, audiosamplerate, audiochannels,
                     audiosamplesize, audiodatarate * 1024);
  }
}

void rtmp_demuxer::init_audio_track(int codecid, int sample_rate, int channels,
                                    int sample_bit, int bit_rate) {
  if (audio_rtmp_decoder_) {
    return;
  }

  if (codecid == 10) {
    codec::aac_track::ptr acc_ptr(new codec::aac_track);
    audio_track_ = acc_ptr;
  }

  if (!audio_track_) {
    throw std::runtime_error("currently only support AAC");
  }

  aac_rtmp_decoder::ptr aac_dec_ptr(new aac_rtmp_decoder(audio_track_));
  audio_rtmp_decoder_ = aac_dec_ptr;
  audio_track_->set_bit_rate(bit_rate);
}

void rtmp_demuxer::init_video_track(int codecid, int bit_rate) {
  if (video_rtmp_decoder_) {
    return;
  }
  // only support h264
  if (codecid == 7) {
    codec::h264_track::ptr avc_ptr(new codec::h264_track);
    video_track_ = avc_ptr;
  }

  if (!video_track_) {
    throw std::runtime_error("currently only support H.264");
  }

  h264_rtmp_decoder::ptr avc_dec_ptr(new h264_rtmp_decoder(video_track_));
  video_rtmp_decoder_ = avc_dec_ptr;
  video_track_->set_bit_rate(bit_rate);
}

void rtmp_demuxer::input_rtmp(const rtmp_packet::ptr &pkt) {
  switch (pkt->msg_type_id) {
  case MSG_VIDEO: {
    if (video_rtmp_decoder_) {
      video_rtmp_decoder_->input_rtmp(pkt);
    }
    break;
  }

  case MSG_AUDIO: {
    if (audio_rtmp_decoder_) {
      audio_rtmp_decoder_->input_rtmp(pkt);
    }
    break;
  }

  default:
    break;
  }
}

} // namespace rtmp