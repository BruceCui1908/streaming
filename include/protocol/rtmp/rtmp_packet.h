#pragma once

#include <cstdint>
#include <cstdlib>

namespace rtmp {

static constexpr size_t RANDOM_HANDSHAKE_SIZE = 1528;

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
  uint8_t body_size[3];
  uint8_t type_id;
  uint8_t stream_index[4];
};

#pragma pack(pop)

} // namespace rtmp