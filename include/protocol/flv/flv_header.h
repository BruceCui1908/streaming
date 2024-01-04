#pragma once

#include <stdint.h>

namespace flv {

// https://zhuanlan.zhihu.com/p/611128149
class flv_header
{
public:
    static constexpr uint8_t kFlvVersion = 1;
    static constexpr uint8_t kFlvHeaderLength = 9;

    char flv[3];

    uint8_t version;

    uint8_t : 5;
    uint8_t have_audio : 1;
    uint8_t : 1;
    uint8_t have_video : 1;

    uint32_t length;
    uint32_t previous_tag_size0;
};

enum tag_type
{
    Script_Data = 18,
    Audio_Data = 8,
    Video_data = 9,
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

} // namespace flv