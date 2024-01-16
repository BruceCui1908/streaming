#include "rtmp_demuxer.h"

#include "codec/aac/aac_track.h"
#include "codec/h264/h264_track.h"
#include <stdexcept>

namespace rtmp {

/// init video/audio tracks based on the metadata passed by the client, eg obs
void rtmp_demuxer::init_tracks_with_metadata(std::unordered_map<std::string, std::any> *metainfo)
{
    if (!metainfo)
    {
        throw std::runtime_error("rtmp demuxer cannot initialize tracks with empty metadata");
    }

    auto &metadata = *metainfo;

    if (metadata.empty())
    {
        throw std::runtime_error("Cannot initialize tracks with empty metadata");
    }

    duration_ = std::any_cast<double>(metadata["duration"]);

    int audiosamplerate = static_cast<int>(std::any_cast<double>(metadata["audiosamplerate"]));

    int audiosamplesize = static_cast<int>(std::any_cast<double>(metadata["audiosamplesize"]));

    int audiochannels = std::any_cast<bool>(metadata["stereo"]) ? 2 : 1;

    int videocodecid = static_cast<int>(std::any_cast<double>(metadata["videocodecid"]));

    int audiocodecid = static_cast<int>(std::any_cast<double>(metadata["audiocodecid"]));

    int audiodatarate = static_cast<int>(std::any_cast<double>(metadata["audiodatarate"]));

    int videodatarate = static_cast<int>(std::any_cast<double>(metadata["videodatarate"]));

    // has video track
    if (videocodecid)
    {
        init_video_track(rtmp_flv_codec_id(videocodecid), videodatarate * 1024);
    }

    if (audiocodecid)
    {
        init_audio_track(rtmp_flv_codec_id(audiocodecid), audiodatarate * 1024);
    }
}

void rtmp_demuxer::init_audio_track(rtmp_flv_codec_id codecid, int bit_rate)
{
    if (audio_rtmp_decoder_)
    {
        return;
    }
    if (codecid != rtmp_flv_codec_id::aac)
    {
        throw std::runtime_error("Currently, only the AAC audio codec is supported");
    }

    audio_track_ = std::make_shared<codec::aac_track>();
    audio_track_->set_bit_rate(bit_rate);
    audio_track_->set_frame_translator(demuxer::get_muxer());
    audio_rtmp_decoder_ = std::make_shared<aac_rtmp_decoder>(std::move(audio_track_));
}

void rtmp_demuxer::init_video_track(rtmp_flv_codec_id codecid, int bit_rate)
{
    if (video_rtmp_decoder_)
    {
        return;
    }

    if (codecid != rtmp_flv_codec_id::h264)
    {
        throw std::runtime_error("Currently, only the H264 video codec is supported");
    }

    video_track_ = std::make_shared<codec::h264_track>();
    video_track_->set_bit_rate(bit_rate);
    video_track_->set_frame_translator(demuxer::get_muxer());
    video_rtmp_decoder_ = std::make_shared<h264_rtmp_decoder>(video_track_);
}

void rtmp_demuxer::input_rtmp(rtmp_packet::ptr pkt)
{
    if (!pkt)
    {
        return;
    }

    pkt->buf()->capture_snapshot();

    if (pkt->msg_type_id == MSG_VIDEO && video_rtmp_decoder_)
    {
        video_rtmp_decoder_->input_rtmp(pkt);
    }
    else if (pkt->msg_type_id == MSG_AUDIO && audio_rtmp_decoder_)
    {
        audio_rtmp_decoder_->input_rtmp(pkt);
    }

    pkt->buf()->restore_snapshot();
}

} // namespace rtmp