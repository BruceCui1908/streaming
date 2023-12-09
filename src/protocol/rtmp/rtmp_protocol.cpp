#include "rtmp_protocol.h"

#include "util/util.h"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace rtmp {
static constexpr size_t C1_HANDSHAKE_SIZE = 1536;
static constexpr size_t kMaxCacheSize = 4 * 1024 * 1024;

rtmp_protocol::rtmp_protocol() {
  // initialize buffer
  pool_ = util::resource_pool<network::buffer_raw>::create();
  next_step_func_ = [this](const char *data, size_t size) {
    return handle_C0C1(data, size);
  };
}

void rtmp_protocol::on_parse_rtmp(const char *data, size_t size) {
  if (cache_data_.unread_length() > kMaxCacheSize) {
    throw std::out_of_range(
        "cached rtmp protocol data length is out of range!");
  }

  const char *dummy = data;
  if (cache_data_.unread_length() > 0) {
    cache_data_.write(data, size);
    data = dummy = cache_data_.data();
    size = cache_data_.unread_length();
  }

  if (!next_step_func_) {
    throw std::runtime_error("rtmp next_step_func is empty!");
  }

  bool needAppend = false;

  const char *index = nullptr;
  do {
    index = next_step_func_(data, size);

    // if returned pointer is null or points to the original ptr, then break
    if (!index || index == data) {
      needAppend = true;
      break;
    }

    size -= index - data;
    data = index;
    if (!size) {
      needAppend = false;
      break;
    }
  } while (index != nullptr);

  if (needAppend) {
    cache_data_.write(data, size);
    return;
  }
}

const char *rtmp_protocol::handle_C0C1(const char *data, size_t size) {
  spdlog::debug("handling C0C1 .....");
  if (size < 1 + C1_HANDSHAKE_SIZE) {
    spdlog::debug("C0C1 handshake needs more data, keep reading!");
    return nullptr;
  }

  // rtmp spec P7
  if (data[0] != 0x03) {
    spdlog::error("invalid rtmp version {}", static_cast<uint8_t>(data[0]));
    throw std::runtime_error("only version 3 is supported!");
  }

#ifdef ENABLE_OPENSSL
  // TODO add ssl support
#else
  // send S0
  char s0 = 0x03;
  send(&s0, 1);

  // send S1
  rtmp_handshake s1;
  send((char *)(&s1), sizeof(s1));

  // send S2
  send(data + 1, C1_HANDSHAKE_SIZE);
#endif

  next_step_func_ = [this](const char *data, size_t size) {
    return handle_C2(data, size);
  };

  return data + 1 + C1_HANDSHAKE_SIZE;
}

const char *rtmp_protocol::handle_C2(const char *data, size_t size) {
  spdlog::debug("handling C0C1 .....");
  if (size < C1_HANDSHAKE_SIZE) {
    spdlog::debug("C2 handshake needs more data, keep reading!");
    return nullptr;
  }

  next_step_func_ = [this](const char *data, size_t size) {
    return split_rtmp(data, size);
  };

  spdlog::debug("rtmp hand shake succeed!");
  return split_rtmp(data + C1_HANDSHAKE_SIZE, size - C1_HANDSHAKE_SIZE);
}

static constexpr size_t HEADER_LENGTH[] = {12, 8, 4, 1};

const char *rtmp_protocol::split_rtmp(const char *data, size_t size) {
  auto ptr = const_cast<char *>(data);
  auto dummy = ptr;

  while (size) {
    size_t offset = 0;
    auto header = reinterpret_cast<rtmp_header *>(ptr);
    auto header_length = HEADER_LENGTH[header->fmt];

    chunk_stream_id_ = header->chunk_id;
    // The IDs 0, 1, and 2 are reserved
    // Values in the range of 3-63 represent the complete stream ID
    switch (chunk_stream_id_) {
    case 0: {
      // Value 0 indicates the 2 byte form and an ID in the range of 64-319 (the
      // second byte + 64)
      if (size < 2) {
        return ptr;
      }

      chunk_stream_id_ = 64 + static_cast<uint8_t>(ptr[1]);
      offset = 1;
      break;
    }

    case 1: {
      // Value 1 indicates the 3 byte form and an ID in the range of 64-65599
      // ((the third byte)*256 + the second byte + 64)
      if (size < 3) {
        return ptr;
      }

      chunk_stream_id_ = 64 + static_cast<uint8_t>(ptr[1]) +
                         ((static_cast<uint8_t>(ptr[2])) << 8);
      offset = 2;
      break;
    }

    default:
      break;
    }

    if (size < offset + header_length) {
      return ptr;
    }

    // reset header
    header = reinterpret_cast<rtmp_header *>(ptr + offset);
    auto &pkt = chunked_data_map_[chunk_stream_id_];
    auto &now_packet = pkt.first;
    auto &last_packet = pkt.second;

    if (!now_packet) {
      now_packet = rtmp_packet::create();
      if (last_packet) {
        // restore the context
        now_packet->restore_context(*last_packet);
      }

      now_packet->is_abs_stamp = false;
    }

    auto &chunk_data = *now_packet;
    chunk_data.chunk_stream_id = chunk_stream_id_;

    switch (header_length) {
    case 12: {
      /**
      This type MUST be used at the start of a chunk stream, and whenever the
      stream timestamp goes backward (e.g., because of a backward seek). The
      absolute timestamp of the message is sent here. If the timestamp is
      greater than or equal to 16777215 (hexadecimal 0xFFFFFF), this field MUST
      be 16777215, indicating the presence of the Extended Timestamp field to
      encode the full 32 bit timestamp. Otherwise, this field SHOULD be the
      entire timestamp
      */
      chunk_data.is_abs_stamp = true;
      chunk_data.ts_delta = util::load_be24(header->time_stamp);
      chunk_data.msg_length = util::load_be24(header->msg_length);
      chunk_data.msg_type_id = header->msg_type_id;
      chunk_data.msg_stream_id = util::load_le32(header->msg_stream_id);
      break;
    }

    case 8: {
      /**
      The message stream ID is not included; this chunk takes the same stream ID
      as the preceding chunk. Streams with variable-sized messages (for example,
      many video formats) SHOULD use this format for the first chunk of each new
      message after the first.
      */
      chunk_data.ts_delta = util::load_be24(header->time_stamp);
      chunk_data.msg_length = util::load_be24(header->msg_length);
      chunk_data.msg_type_id = header->msg_type_id;
      break;
    }

    case 4: {
      /**
      Neither the stream ID nor the message length is included; this chunk has
      the same stream ID and message length as the preceding chunk. Streams with
      constant-sized messages (for example, some audio and data formats) SHOULD
      use this format for the first chunk of each message after the first.
      */
      chunk_data.ts_delta = util::load_be24(header->time_stamp);
      break;
    };

      // When a single message is split into chunks, all chunks of a message
      // except the first one SHOULD use type 3 packet

    default:
      break;
    }

    auto time_stamp = chunk_data.ts_delta;
    if (chunk_data.ts_delta == 0xFFFFFF) {
      if (size < header_length + offset + 4) {
        return ptr;
      }

      time_stamp = util::load_be32(ptr + offset + header_length);
      offset += 4;
    }

    auto remain_msg_len =
        chunk_data.msg_length - chunk_data.buffer.unread_length();
    auto more = std::min(chunk_size_in_, (size_t)(remain_msg_len));
    if (size < header_length + offset + more) {
      return ptr;
    }

    if (more) {
      chunk_data.buffer.write(data + header_length + offset, more);
    }

    ptr += header_length + offset + more;
    size -= header_length + offset + more;

    if (chunk_data.msg_length == chunk_data.buffer.unread_length()) {
      spdlog::debug("rtmp frame is ready");
      msg_stream_id_ = chunk_data.msg_stream_id;
      chunk_data.time_stamp =
          time_stamp + (chunk_data.is_abs_stamp ? 0 : chunk_data.time_stamp);
      // keep the context
      last_packet = now_packet;
      if (chunk_data.msg_length) {
        handle_chunk(std::move(now_packet));
      } else {
        now_packet = nullptr;
      }
    }
  }

  return ptr;
}

void rtmp_protocol::handle_chunk(rtmp_packet::ptr ptr) {
  if (!ptr) {
    spdlog::error("handle_chunk received empty packet");
    return;
  }

  switch (ptr->msg_type_id) {
  case MSG_ACK: {
    // The client or the server MUST send an acknowledgment to the peer after
    // receiving bytes equal to the window size. The window size is the maximum
    // number of bytes that the sender sends without receiving acknowledgment
    // from the receiver
    if (ptr->buffer.unread_length() < 4) {
      throw std::runtime_error("MSG_ACK not enough data");
    }
    spdlog::debug("received MSG_ACK");
    break;
  }

  case MSG_CMD:
  case MSG_CMD3: {
    AMFDecoder dec(ptr->buffer, ptr->msg_type_id == MSG_CMD ? 0 : 3);
    on_process_cmd(dec);
    break;
  }

  default:
    break;
  }
}

void rtmp_protocol::on_process_cmd(AMFDecoder &dec) {
  // member function pointer
  typedef void (rtmp_protocol::*cmd_func)(AMFDecoder &);
  static std::unordered_map<std::string, cmd_func> cmd_funcs = {
      {"connect", &rtmp_protocol::on_amf_connect}};

  std::string method = dec.load<std::string>();

  spdlog::debug("cmd method = {}", method);

  auto it = cmd_funcs.find(method);
  if (it == cmd_funcs.end()) {
    spdlog::debug("cannot find handler for method = {}", method);
    return;
  }

  recv_req_id_ = dec.load<double>();
  auto func = it->second;
  (this->*func)(dec);
}

void rtmp_protocol::on_amf_connect(AMFDecoder &dec) {
  auto params = dec.load_object();
  for (auto &it : params.get_map()) {
    spdlog::debug("key = {}, value = {}", it.first, it.second.as_string());
  }

  // TODO
}

void rtmp_protocol::send_rtmp(uint8_t msg_type_id, uint32_t msg_stream_id,
                              const std::string &data, uint32_t time_stamp,
                              int chunk_stream_id) {
  // only send type 1 chunk basic header, which means chunk stream id must be
  // fall into 2-63
  if (chunk_stream_id < 2 || chunk_stream_id > 63) {
    throw std::runtime_error("only support type 1 chunk header");
  }

  auto chunk_header = pool_->obtain();
  chunk_header->set_capacity(sizeof(rtmp_header));
  chunk_header->set_capacity(sizeof(rtmp_header));
  // convert to rtmp_header
  rtmp_header *header = reinterpret_cast<rtmp_header *>(chunk_header->data());
  header->fmt = 0;
  header->chunk_id = chunk_stream_id;
  header->msg_type_id = msg_type_id;

  bool has_extended_timestamp = time_stamp >= 0xFFFFFF;
  util::set_be24(header->time_stamp,
                 has_extended_timestamp ? 0xFFFFFF : time_stamp);
  util::set_be24(header->msg_length, static_cast<uint32_t>(data.size()));
  util::set_le32(header->msg_stream_id, msg_stream_id);

  // send rtmp header
  send(chunk_header->data(), chunk_header->size());

  // for extended timestamp
  char extended_timestamp[4] = {0};
  if (has_extended_timestamp) {
    util::set_be32(extended_timestamp, time_stamp);
  }

  // type 3 header for sending remaining bytes
  char header_pkt = 0;
  header_pkt |= chunk_stream_id;
  header_pkt |= 3 << 6;

  auto ptr = data.data();
  size_t offset = 0;
  size_t total_size = sizeof(rtmp_header);

  while (offset < data.size()) {
    // if offset > 0 && < data.size(), need to send remain data with type 3
    // header
    if (offset) {
      send(&header_pkt, 1);
      total_size += 1;
    }

    if (has_extended_timestamp) {
      send(extended_timestamp, sizeof(extended_timestamp));
      total_size += 4;
    }

    size_t chunk_size = std::min(chunk_size_out_, data.size() - offset);
    send(ptr, chunk_size);
    total_size += chunk_size;
    offset += chunk_size;
    ptr += chunk_size;
  }

  bytes_sent_ += static_cast<uint32_t>(total_size);
  if (windows_size_ > 0 && bytes_sent_ - bytes_sent_last_ >= windows_size_) {
    bytes_sent_last_ = bytes_sent_;
    send_acknowledgement(bytes_sent_);
  }
}

void rtmp_protocol::send_acknowledgement(uint32_t size) {
  size = htonl(size);
  std::string ack((char *)&size, sizeof(uint32_t));
  send_rtmp(MSG_ACK, msg_stream_id_, ack, 0, CHUNK_NETWORK);
}

void rtmp_protocol::set_chunk_size(uint32_t size) {
  uint32_t len = htonl(size);
  std::string set_chunk((char *)&len, 4);
  send_rtmp(MSG_SET_CHUNK, msg_stream_id_, set_chunk, 0, CHUNK_NETWORK);
  chunk_size_out_ = size;
}

void rtmp_protocol::set_peer_bandwidth(uint32_t size) {
  size = htonl(size);
  std::string set_peer_bd((char *)&size, 4);
  set_peer_bd.push_back((char)0x02);
  send_rtmp(MSG_SET_PEER_BW, msg_stream_id_, set_peer_bd, 0, CHUNK_NETWORK);
}

} // namespace rtmp