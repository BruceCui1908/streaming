#pragma once

#include "session.h"

namespace network {

class session_receiver
{
public:
    session_receiver() = default;
    virtual ~session_receiver() = default;

    virtual session::ptr get_session() = 0;
};

} // namespace network