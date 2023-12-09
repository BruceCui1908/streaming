#pragma once

#include "network/flat_buffer.h"

#include <cstdint>
#include <cstdlib>
#include <memory>

namespace rtmp {

static constexpr size_t RANDOM_HANDSHAKE_SIZE = 1528;
static constexpr size_t DEFAULT_CHUNK_LEN = 128;

#define MSG_SET_CHUNK 1    /*Set Chunk Size (1)*/
#define MSG_ABORT 2        /*Abort Message (2)*/
#define MSG_ACK 3          /*Acknowledgement (3)*/
#define MSG_USER_CONTROL 4 /*User Control Messages (4)*/
#define MSG_WIN_SIZE 5     /*Window Acknowledgement Size (5)*/
#define MSG_SET_PEER_BW 6  /*Set Peer Bandwidth (6)*/
#define MSG_AUDIO 8        /*Audio Message (8)*/
#define MSG_VIDEO 9        /*Video Message (9)*/
#define MSG_DATA 18        /*Data Message (18, 15) AMF0*/
#define MSG_DATA3 15       /*Data Message (18, 15) AMF3*/
#define MSG_CMD 20         /*Command Message AMF0 */
#define MSG_CMD3 17        /*Command Message AMF3 */
#define MSG_OBJECT3 16     /*Shared Object Message (19, 16) AMF3*/
#define MSG_OBJECT 19      /*Shared Object Message (19, 16) AMF0*/
#define MSG_AGGREGATE 22   /*Aggregate Message (22)*/

#define CHUNK_NETWORK 2 /*网络相关的消息(参见 Protocol Control Messages)*/
#define CHUNK_SYSTEM 3 /*向服务器发送控制消息(反之亦可)*/
#define CHUNK_CLIENT_REQUEST_BEFORE                                            \
  3 /*客户端在createStream前,向服务器发出请求的chunkID*/
#define CHUNK_CLIENT_REQUEST_AFTER                                             \
  4 /*客户端在createStream后,向服务器发出请求的chunkID*/
#define CHUNK_AUDIO 6 /*音频chunkID*/
#define CHUNK_VIDEO 7 /*视频chunkID*/

#pragma pack(push, 1)

/// rtmp spec P8
struct rtmp_handshake {
  rtmp_handshake();

  uint8_t time_stamp_[4] = {0};
  uint8_t zero_[4] = {0};
  uint8_t random_[RANDOM_HANDSHAKE_SIZE];
};

/// rtmp spec P14
struct rtmp_header {
#if __BYTE_ORDER == __BIG_ENDIAN
  uint8_t fmt : 2;
  uint8_t chunk_id : 6;
#else
  uint8_t chunk_id : 6;
  uint8_t fmt : 2;
#endif
  uint8_t time_stamp[3];
  uint8_t msg_length[3];
  uint8_t msg_type_id;
  uint8_t msg_stream_id[4];
};

#pragma pack(pop)

class rtmp_packet : public std::enable_shared_from_this<rtmp_packet> {
public:
  using ptr = std::shared_ptr<rtmp_packet>;

  bool is_abs_stamp{false};
  uint32_t chunk_stream_id;
  uint32_t msg_stream_id;
  uint32_t msg_length;
  uint8_t msg_type_id;
  uint32_t ts_delta;
  uint32_t time_stamp;

  network::flat_buffer buffer;

  ~rtmp_packet() = default;
  rtmp_packet(const rtmp_packet &) = delete;
  rtmp_packet &operator=(const rtmp_packet &) = delete;
  rtmp_packet(rtmp_packet &&) = delete;
  rtmp_packet &operator=(rtmp_packet &&) = delete;

  rtmp_packet &restore_context(const rtmp_packet &);

public:
  static ptr create();

private:
  rtmp_packet() = default;
};

} // namespace rtmp