#pragma once

#include "network/session.h"
#include "http_protocol.h"
#include "util/timer.h"

namespace http {

class http_session : public network::session, public http_protocol
{
public:
    using ptr = std::shared_ptr<http_session>;

    static ptr create(SESSION_CONSTRUCTOR_PARAMS);

    ~http_session() override = default;

    void start() override;

private:
    http_session(SESSION_CONSTRUCTOR_PARAMS);

    void on_recv(network::flat_buffer &) override;

private:
    // ticker
    util::ticker ticker_;
};
} // namespace http