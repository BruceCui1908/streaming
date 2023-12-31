#include "aac_rtmp.h"
#include "aac/aac_frame.h"
#include "aac/aac_track.h"

namespace rtmp {

/// https://zhuanlan.zhihu.com/p/649512028?utm_id=0
void aac_rtmp_decoder::input_rtmp(rtmp_packet::ptr &pkt)
{
    // aac[0] and aac[1]
    const auto &buf = pkt->buf();
    if (!buf)
    {
        throw std::runtime_error("rtmp_packet has empty aac data");
    }

    buf->require_length_or_fail(2);

    const auto track_ptr = get_track();
    if (!track_ptr)
    {
        throw std::runtime_error("The audio track must be attached to the "
                                 "aac_rtmp_decoder prior to processing RTMP");
    }

    auto aac_track_ptr = std::dynamic_pointer_cast<codec::aac_track>(track_ptr);
    if (!aac_track_ptr)
    {
        throw std::runtime_error("The video track cannot be cast to aac track in aac_rtmp_decoder");
    }

    if (pkt->is_config_frame())
    {
        buf->consume_or_fail(2);
        aac_track_ptr->parse_config(*buf);
        return;
    }

    buf->consume_or_fail(2);

    /// Based on testing, neither OBS nor FFmpeg would send AAC with ADTS header.
    auto adts_head = aac_track_ptr->extract_aac_config();
    adts_head.aac_frame_length = buf->unread_length() + codec::kAdtsHeaderLength;
    auto adts_raw = aac_track_ptr->dump_adts_header(adts_head);

    auto aac_data_buf = network::flat_buffer::create(adts_raw.size() + buf->unread_length());
    aac_data_buf->write(adts_raw.data(), adts_raw.size());
    aac_data_buf->write(buf->data(), buf->unread_length());

    codec::aac_frame af(aac_data_buf, pkt->time_stamp);
    track_ptr->input_frame(af);
}

} // namespace rtmp