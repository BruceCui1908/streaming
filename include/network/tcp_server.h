#pragma once

#include "server.h"
#include "session.h"

#include <boost/asio.hpp>

namespace network {

using boost::asio::ip::tcp;

#define TCP_SERVER_PARAMS boost::asio::io_context &io_context, uint16_t port, Ip_Type ip_type

class tcp_server : public server
{
public:
    using ptr = std::shared_ptr<tcp_server>;

    static constexpr char kDefaultLocalIpv4[] = "0.0.0.0";
    static constexpr char kDefaultLocalIpv6[] = "0::0";

    static ptr create(TCP_SERVER_PARAMS = Ip_Type::ipv4);

    ~tcp_server();

    const std::string &info() override;

    void restart() override;

    template<typename SessionProtocol, typename = std::enable_if_t<std::is_base_of_v<session, SessionProtocol>>>
    void start()
    {
        session_alloc_ = [this](boost::asio::ip::tcp::socket sock) {
            if (!session_manager_)
            {
                session_manager_ = session_manager::create(info());
            }

            auto session_ptr = SessionProtocol::create(std::move(sock), session_manager_->generate_session_prefix(), session_manager_);
            session_manager_->add(session_ptr);
            return session_ptr;
        };

        do_accept();
    }

private:
    tcp_server(TCP_SERVER_PARAMS);

    // callback after receiving termination signal
    void start_signal_listener();

    void do_accept();

private:
    boost::asio::io_context &io_context_;

    /// The signal_set is used to register for process termination notifications.
    boost::asio::signal_set signals_;

    /// Acceptor used to listen for incoming connections.
    tcp::acceptor acceptor_;

    // socket fd of this tcp server
    int raw_fd_{-1};

    std::string server_name_;

    session_manager::ptr session_manager_;

    std::function<session::ptr(boost::asio::ip::tcp::socket)> session_alloc_;
};

} // namespace network