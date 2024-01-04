#include "flv_muxer.h"
#include "rtmp/amf.h"
#include "util/util.h"

#include <arpa/inet.h>
#include <cstring>

namespace flv {

flv_muxer::flv_muxer()
{
    pool_ = util::resource_pool<network::buffer_raw>::create();
}

flv_muxer::ptr flv_muxer::create()
{
    return std::shared_ptr<flv_muxer>(new flv_muxer);
}

void flv_muxer::start_muxing(network::socket_sender *sender, rtmp::rtmp_media_source::ptr &rtmp_src_ptr, uint32_t start_pts)
{
    if (!sender || !rtmp_src_ptr)
    {
        throw std::runtime_error("flv cannot mux with invalid sources");
    }

    // 1. send flv header
    auto header_ptr = prepare_flv_header();
    sender->send(header_ptr->data(), header_ptr->size(), true);

    // 2. send metadata
    auto &metadata = *rtmp_src_ptr->get_meta_data_or_fail();
    rtmp::AMFEncoder encoder;
    encoder << "onMetaData" << metadata;
    write_flv(sender, Script_Data, encoder.c_str(), encoder.size());

    // 3. send config frame
    rtmp_src_ptr->loop_config_frame([this, sender](const rtmp::rtmp_packet::ptr &ptr) {
        if (!this || !sender || (!ptr->is_audio_pkt() && !ptr->is_video_pkt()))
        {
            return;
        }

        write_flv(sender, ptr->is_audio_pkt() ? Audio_Data : Video_data, ptr->buf()->data(), ptr->buf()->unread_length(), ptr->time_stamp);
    });

    // TODO
}

network::buffer_raw::ptr flv_muxer::prepare_flv_header()
{
    auto buffer_ptr = pool_->obtain();
    buffer_ptr->set_capacity(sizeof(flv_header));
    buffer_ptr->set_size(sizeof(flv_header));

    flv_header *header = reinterpret_cast<flv_header *>(buffer_ptr->data());
    std::memset(header, 0, sizeof(flv_header));

    header->flv[0] = 'F';
    header->flv[1] = 'L';
    header->flv[2] = 'V';
    header->version = flv_header::kFlvVersion;
    header->length = flv_header::kFlvHeaderLength;
    header->have_audio = true;
    header->have_video = true;

    return buffer_ptr;
}

network::buffer_raw::ptr flv_muxer::prepare_flv_tag_header(tag_type t, size_t body_size, uint32_t time_stamp)
{
    auto buffer_ptr = pool_->obtain();
    buffer_ptr->set_capacity(sizeof(flv_tag_header));
    buffer_ptr->set_size(sizeof(flv_tag_header));

    flv_tag_header *header = reinterpret_cast<flv_tag_header *>(buffer_ptr->data());
    std::memset(header, 0, sizeof(flv_tag_header));

    header->type = t;
    util::set_be24(header->data_size, static_cast<uint32_t>(body_size));
    header->timestamp_ex = (time_stamp >> 24) & 0xFF;
    util::set_be24(header->timestamp, time_stamp & 0xFFFFFF);

    return buffer_ptr;
}

void flv_muxer::write_flv(network::socket_sender *sender, tag_type t, const char *data, size_t size, uint32_t time_stamp)
{
    if (!sender || !data || !size)
    {
        throw std::runtime_error("cannot write flv with invalid source or data");
    }

    auto tag_header_ptr = prepare_flv_tag_header(t, size, time_stamp);
    // send header
    sender->send(tag_header_ptr->data(), tag_header_ptr->size(), true);
    // send body
    sender->send(data, size, true);
    // send tag
    uint32_t total_size = htonl(static_cast<uint32_t>(tag_header_ptr->size() + size));
    auto previous_tag = std::to_string(total_size);
    sender->send(previous_tag.c_str(), previous_tag.size(), true);
}

} // namespace flv