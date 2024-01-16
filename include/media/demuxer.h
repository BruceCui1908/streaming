#pragma once

#include "muxer.h"

namespace media {

class demuxer
{
public:
    demuxer() = default;
    virtual ~demuxer() = default;

    void set_muxer(muxer::ptr mux)
    {
        if (!mux)
        {
            throw std::runtime_error("Unable to assign an empty muxer to the demuxer.");
        }

        muxer_ = std::move(mux);
    }

    const muxer::ptr get_muxer()
    {
        if (!muxer_)
        {
            throw std::runtime_error("The muxer assigned to the demuxer has been lost.");
        }
        return muxer_;
    }

private:
    muxer::ptr muxer_;
};

} // namespace media