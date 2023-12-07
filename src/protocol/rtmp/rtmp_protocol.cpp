#include "rtmp_protocol.h"

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

    // TODO

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
    return handle_rtmp(data, size);
  };

  spdlog::debug("rtmp hand shake succeed!");
  return handle_rtmp(data + C1_HANDSHAKE_SIZE, size - C1_HANDSHAKE_SIZE);
}

static constexpr size_t HEADER_LENGTH[] = {12, 8, 4, 1};

const char *rtmp_protocol::handle_rtmp(const char *data, size_t size) {
  auto dummy = data;
  while (size) {
    size_t offset - 0;
    auto header = reinterpret_cast<rtmp_header *>(data);
    auto header_length = HEADER_LENGTH[header->fmt];

    chunk_stream_id_ = header->chunk_id;
    switch (chunk_stream_id_) {
    case 0: {
      // Value 0 indicates the 2 byte form and an ID in the range of 64-319 (the
      // second byte + 64)
      if (size < 2) {
        return data;
      }

      chunk_stream_id_ = 64 + static_cast<uint8_t>(data[1]);
      offset = 1;
      break;
    }

    case 1: {
      // Value 1 indicates the 3 byte form and an ID in the range of 64-65599
      // ((the third byte)*256 + the second byte + 64)
      if (size < 3) {
        return data;
      }

      chunk_stream_id_ = 64 + static_cast<uint8_t>(data[1]) +
                         ((static_cast<uint8_t>(data[2])) << 8);
      offset = 2;
      break;
    }

    default:
      break;
    }

    if (size < offset + header_length) {
      return data;
    }

    // reset header
    header = reinterpret_cast<rtmp_header *>(data + offset);
  }

  // TODO
  return nullptr;
}

} // namespace rtmp