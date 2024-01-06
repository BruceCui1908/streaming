#include "tcp_server.h"

#include <csignal>

namespace network {

tcp_server::ptr tcp_server::create(TCP_SERVER_PARAMS)
{
    return std::shared_ptr<tcp_server>(new tcp_server(io_context, port, ip_type));
}

tcp_server::tcp_server(TCP_SERVER_PARAMS)
    : server(port, Sock_Type::tcp, ip_type)
    , io_context_{io_context}
    , signals_{io_context_}
    , acceptor_{io_context_}
{

    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    // register ctrl-c and kill
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
    start_signal_listener();

    tcp::resolver resolver(io_context_);
    auto result = resolver.resolve(ip_type_ == network::Ip_Type::ipv4 ? kDefaultLocalIpv4 : kDefaultLocalIpv6, std::to_string(port));
    if (result.empty())
    {
        throw std::invalid_argument("cannot resolve tcp address");
    }

    auto endpoint = result.begin()->endpoint();
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.non_blocking(true);
    acceptor_.bind(endpoint);
    acceptor_.listen();

    raw_fd_ = acceptor_.native_handle();
    session_manager_ = session_manager::create(info());

    spdlog::info("Server {} created", info());
}

tcp_server::~tcp_server()
{
    spdlog::info("Server {} destroyed", info());
}

void tcp_server::do_accept()
{
    if (!acceptor_.is_open())
    {
        return;
    }

    acceptor_.async_accept(boost::asio::make_strand(io_context_), [this](boost::system::error_code ec, tcp::socket sock) {
        // Check whether the server was stopped by a signal before this
        // completion handler had a chance to run.
        if (!acceptor_.is_open())
        {
            return;
        }

        if (!ec)
        {
            // create new session and start
            auto new_session = session_alloc_(std::move(sock));
            new_session->start();
        }
        else
        {
            spdlog::error("acceptor received error {}, msg = {}", ec.value(), ec.message());
        }

        do_accept();
    });
}

void tcp_server::restart()
{
    do_accept();
}

const std::string &tcp_server::info()
{
    if (server_name_.empty())
    {
        server_name_ = fmt::format("TCP[{}|{}|{}]", port_, ip_type_ == Ip_Type::ipv4 ? "ipv4" : "ipv6", raw_fd_);
    }
    return server_name_;
}

void tcp_server::start_signal_listener()
{
    signals_.async_wait([this](boost::system::error_code, int signo) {
        spdlog::info("{} received signal {}", info(), signo);
        // close the acceptor
        acceptor_.close();

        // close io_context
        io_context_.stop();

        // close the session map
        session_manager_->stop_all();
    });
}

} // namespace network