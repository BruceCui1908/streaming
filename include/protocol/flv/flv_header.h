#pragma once

#include <stdint.h>

namespace flv {

#pragma pack(push, 1)

// https://zhuanlan.zhihu.com/p/611128149
class flv_header
{
public:
    static constexpr uint8_t kFlvVersion = 1;
    static constexpr uint32_t kFlvHeaderLength = 9;

    char flv[3];

    uint8_t version;
#if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t : 5;
    uint8_t have_audio : 1;
    uint8_t : 1;
    uint8_t have_video : 1;
#else
    uint8_t have_video : 1;
    uint8_t : 1;
    uint8_t have_audio : 1;
    uint8_t : 5;
#endif

    uint32_t length;
};

class flv_tag_header
{
public:
    uint8_t type = 0;
    uint8_t data_size[3] = {0};
    uint8_t timestamp[3] = {0};
    uint8_t timestamp_ex = 0;
    uint8_t stream_id[3] = {0};
};

#pragma pack(pop)

enum class tag_type : uint8_t
{
    Script_Data = 18,
    Audio_Data = 8,
    Video_data = 9,
};

} // namespace flv