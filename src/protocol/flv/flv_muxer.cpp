#include "flv_muxer.h"
#include "rtmp/amf.h"
#include "util/util.h"

namespace flv {

flv_muxer::flv_muxer()
{
    pool_ = util::resource_pool<network::buffer_raw>::create();
}

flv_muxer::ptr flv_muxer::create()
{
    return std::shared_ptr<flv_muxer>(new flv_muxer);
}

flv_muxer::~flv_muxer()
{
    if (auto strong_client_reader = weak_client_reader_.lock())
    {
        strong_client_reader->leave();
    }
}

void flv_muxer::start_muxing(network::socket_sender *sender, std::weak_ptr<network::session> weak_session,
    rtmp::rtmp_media_source::ptr &rtmp_src_ptr, const http::http_flv_header::ptr &flv_header_ptr, uint32_t start_pts)
{
    if (!sender || !rtmp_src_ptr || !flv_header_ptr)
    {
        throw std::runtime_error("flv cannot mux with invalid sources");
    }

    // 1. send flv header
    auto header_ptr = prepare_flv_header();
    sender->send(header_ptr->data(), header_ptr->size(), true);
    char first_tag_size[4] = {0};
    sender->send(first_tag_size, sizeof(first_tag_size), true);

    // 2. send metadata
    auto &metadata = *rtmp_src_ptr->get_metadata();
    rtmp::AMFEncoder encoder;
    encoder << "onMetaData" << metadata;
    write_flv(sender, tag_type::Script_Data, encoder.c_str(), encoder.size());

    // 3. send config frame
    rtmp_src_ptr->loop_config_frame([this, sender](const rtmp::rtmp_packet::ptr &ptr) {
        if (!this || !sender || (!ptr->is_audio_pkt() && !ptr->is_video_pkt()))
        {
            return;
        }

        write_flv(sender, ptr->is_audio_pkt() ? tag_type::Audio_Data : tag_type::Video_data, ptr->buf()->data(),
            ptr->buf()->unread_length(), ptr->time_stamp);
    });

    auto pkt_dispatcher = rtmp_src_ptr->get_dispatcher();

    // 4. create client_reader
    auto client_reader_ptr = media::client_reader<rtmp::rtmp_packet::ptr>::create(weak_session, pkt_dispatcher, flv_header_ptr->token());

    // 5. set read_cb
    std::weak_ptr<flv_muxer> weak_self = shared_from_this();

    client_reader_ptr->set_read_cb([sender, start_pts, weak_self](const rtmp::rtmp_packet::ptr &pkt) {
        if (start_pts > 0 && pkt->time_stamp < start_pts)
        {
            return;
        }

        auto strong_self = weak_self.lock();
        if (!strong_self)
        {
            return;
        }

        strong_self->write_flv(sender, tag_type(pkt->msg_type_id), pkt->buf()->data(), pkt->buf()->unread_length(), pkt->time_stamp);
    });

    client_reader_ptr->set_detach([weak_session](bool is_normal) {
        if (auto strong_session = weak_session.lock())
        {
            strong_session->shutdown();
        }
    });

    pkt_dispatcher->regist_reader(client_reader_ptr);

    weak_client_reader_ = client_reader_ptr;
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
    header->length = htonl(flv_header::kFlvHeaderLength);
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

    header->type = magic_enum::enum_integer(t);
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
    char size_arr[4];
    std::memcpy(size_arr, &total_size, sizeof(total_size));
    sender->send(size_arr, sizeof(size_arr), true);
}

} // namespace flv