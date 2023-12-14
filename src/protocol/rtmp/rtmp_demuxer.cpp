#include "rtmp_demuxer.h"

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

  // TODO trigger add complete event
}

void rtmp_demuxer::init_audio_track(int codecid, int sample_rate, int channels,
                                    int sample_bit, int bit_rate) {
  if (audio_rtmp_decoder_) {
    return;
  }

  // TODO
}

void rtmp_demuxer::init_video_track(int codecid, int bit_rate) {
  if (video_rtmp_decoder_) {
    return;
  }
  // only support h264
  if (codecid == 7) {
    video_track_ = std::shared_ptr<codec::h264_track>(new codec::h264_track);
  }

  if (!video_track_) {
    throw std::runtime_error("currently only support H.264");
  }

  video_rtmp_decoder_ =
      std::shared_ptr<h264_rtmp_decoder>(new h264_rtmp_decoder(video_track_));

  video_track_->set_bit_rate(bit_rate);

  // TODO add track
}

void rtmp_demuxer::input_rtmp(const rtmp_packet::ptr &pkt) {
  // TODO
}

} // namespace rtmp