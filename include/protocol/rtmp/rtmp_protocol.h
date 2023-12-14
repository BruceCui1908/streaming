#pragma once

#include "amf.h"
#include "media/media_info.h"
#include "network/buffer.h"
#include "network/flat_buffer.h"
#include "rtmp_media_source.h"
#include "rtmp_packet.h"
#include "util/resource_pool.h"

#include <any>
#include <unordered_map>
#include <utility>

namespace rtmp {
class rtmp_protocol {
public:
  virtual ~rtmp_protocol() = default;

protected:
  rtmp_protocol();

  void on_parse_rtmp(const char *, size_t);

  virtual void send(const char *, size_t, bool is_async = false,
                    bool is_close = false) = 0;

private:
  const char *handle_C0C1(const char *, size_t);
  const char *handle_C2(const char *, size_t);
  const char *split_rtmp(const char *, size_t);
  void handle_chunk(rtmp_packet::ptr);

  void on_process_cmd(AMFDecoder &);
  void on_amf_connect(AMFDecoder &);
  void on_amf_createStream(AMFDecoder &);
  void on_amf_publish(AMFDecoder &);
  void on_process_metadata(AMFDecoder &);

  void send_rtmp(uint8_t msg_type_id, uint32_t msg_stream_id,
                 const std::string &data, uint32_t time_stamp,
                 int chunk_stream_id);

  void send_acknowledgement(uint32_t);
  void set_chunk_size(uint32_t);
  void set_peer_bandwidth(uint32_t);

private:
  // P13
  int chunk_stream_id_{0};
  int msg_stream_id_{0};

  size_t chunk_size_in_ = DEFAULT_CHUNK_LEN;
  size_t chunk_size_out_ = DEFAULT_CHUNK_LEN;

  std::unordered_map<
      int, std::pair<rtmp_packet::ptr /* now */, rtmp_packet::ptr /* last */>>
      chunked_data_map_{};

  // for sending rtmp packet
  util::resource_pool<network::buffer_raw>::ptr pool_;
  network::flat_buffer cache_data_;
  std::function<const char *(const char *, size_t)> next_step_func_;

  double transaction_id_ = 0;

  // Acknowledgement
  uint64_t bytes_sent_{0};
  uint64_t bytes_sent_last_{0};
  uint64_t bytes_recv_{0};
  uint64_t bytes_recv_last_{0};
  uint32_t windows_size_{0};

  // media info
  media::media_info::ptr media_info_;

  // media source
  rtmp_media_source::ptr rtmp_source_;

  // ownership
  std::shared_ptr<void> src_ownership_;

  // store metadata
  std::unordered_map<std::string, std::any> meta_data_{};
};

} // namespace rtmp