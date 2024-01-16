#pragma once

#include "codec/frame.h"

namespace codec {

class aac_frame : public codec::frame
{
public:
    using ptr = std::shared_ptr<aac_frame>;

    /// @param  ptr to underlying data
    /// @param  dts
    aac_frame(network::flat_buffer::ptr, uint64_t);

    bool is_key_frame() const override
    {
        return false;
    }
    bool is_config_frame() const override
    {
        return false;
    }
};

} // namespace codec