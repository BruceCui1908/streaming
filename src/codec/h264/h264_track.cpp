#include "codec/h264/h264_track.h"
#include "SPSParser.h"

namespace codec {

void h264_track::extract_bitstream_sps()
{
    if (sps_.empty() || sps_.size() < 4)
    {
        spdlog::error("sps {} is not valid", sps_);
        return;
    }

    T_GetBitContext tGetBitBuf;
    T_SPS tH264SpsInfo;
    std::memset(&tGetBitBuf, 0, sizeof(tGetBitBuf));
    std::memset(&tH264SpsInfo, 0, sizeof(tH264SpsInfo));

    tGetBitBuf.pu8Buf = reinterpret_cast<uint8_t *>(sps_.data()) + 1;
    tGetBitBuf.iBufSize = (int)(sps_.size() - 1);
    if (0 != h264DecSeqParameterSet(static_cast<void *>(&tGetBitBuf), &tH264SpsInfo))
    {
        throw std::runtime_error("cannot parse sps");
    }

    h264GetWidthHeight(&tH264SpsInfo, &width_, &height_);
    h264GeFramerate(&tH264SpsInfo, &fps_);

    spdlog::debug("successfully parsed h264 sps, video width = {}, height = {}, fps = {}", width_, height_, fps_);
}

// https://www.jianshu.com/p/4f95617f30d0
void h264_track::parse_config(network::flat_buffer &buf)
{
    // byte[0] version
    // byte[1] avc profile
    // byte[2] avc compatibility
    // byte[3] avc level
    // byte[4] FF
    // byte[5] E1
    buf.consume_or_fail(6);

    // byte[6] byte[7] sps length
    auto sps_size = buf.read_uint16();
    buf.require_length_or_fail(sps_size);

    sps_.assign(buf.data(), sps_size);
    buf.consume_or_fail(sps_size);

    // skip the byte 01
    buf.consume_or_fail(1);

    auto pps_size = buf.read_uint16();
    buf.require_length_or_fail(pps_size);

    pps_.assign(buf.data(), pps_size);
    buf.consume_or_fail(pps_size);

    extract_bitstream_sps();

    // pass sps frame to muxer
    encapsulate_config_frame(sps_);

    // pass pps frame to muxer
    encapsulate_config_frame(pps_);
}

void h264_track::encapsulate_config_frame(const std::string &config)
{
    static_assert(sizeof(kH264HeaderPrefix) == 5);
    if (config.empty())
    {
        return;
    }

    auto config_frame_len = sizeof(kH264HeaderPrefix) - 1 + config.size();
    auto config_ptr = network::flat_buffer::create(config_frame_len);
    config_ptr->write(kH264HeaderPrefix, sizeof(kH264HeaderPrefix) - 1);
    config_ptr->write(config.data(), config.size());

    h264_frame config_frame;
    config_frame.set_prefix_size(4);
    config_frame.set_dts(0);
    config_frame.set_pts(0);
    config_frame.set_data(config_ptr);
    track::input_frame(config_frame);
}

} // namespace codec