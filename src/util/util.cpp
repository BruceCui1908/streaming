#include "util.h"

namespace util {
uint64_t current_micros()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

uint64_t current_millis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

uint32_t load_le32(const void *p)
{
    const uint8_t *data = static_cast<const uint8_t *>(p);
    return ((uint32_t)data[3] << 24) | ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | ((uint32_t)data[0]);
}

uint32_t load_be24(const void *p)
{
    const uint8_t *data = static_cast<const uint8_t *>(p);
    return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | ((uint32_t)data[2]);
}

uint32_t load_be32(const void *p)
{
    const uint8_t *data = static_cast<const uint8_t *>(p);
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | ((uint32_t)data[3]);
}

void set_be24(void *p, uint32_t val)
{
    uint8_t *data = static_cast<uint8_t *>(p);
    data[0] = val >> 16;
    data[1] = val >> 8;
    data[2] = val;
}

void set_be32(void *p, uint32_t val)
{
    uint8_t *data = static_cast<uint8_t *>(p);
    data[0] = val >> 24;
    data[1] = val >> 16;
    data[2] = val >> 8;
    data[3] = val;
}

void set_le32(void *p, uint32_t val)
{
    uint8_t *data = static_cast<uint8_t *>(p);
    data[3] = val >> 24;
    data[2] = val >> 16;
    data[1] = val >> 8;
    data[0] = val;
}

} // namespace util