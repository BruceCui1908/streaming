#include "session.h"

namespace network {

// session
session::session(SESSION_CONSTRUCTOR_PARAMS)
    : socket_{std::move(sock)}
    , session_prefix_{session_prefix}
    , raw_fd_{socket_.native_handle()}
    , session_manager_{manager}
{}

const std::string &session::id()
{
    if (id_.empty())
    {
        id_ = fmt::format("{}-client_socket({})", session_prefix_, raw_fd_);
    }
    return id_;
}

session::~session()
{
    spdlog::debug("{} destroyed!", id());
}

void session::do_read()
{
    if (!socket_.is_open())
    {
        spdlog::debug("{} socket has been closed, return from do_read()", id());
        return;
    }

    std::weak_ptr<session> weak_self = shared_from_this();
    socket_.async_read_some(boost::asio::buffer(buffer_.write_begin(), buffer_.socket_read_length(kSocketReadSize)),
        [this, weak_self](boost::system::error_code ec, size_t bytes_transferred) {
            auto strong_self = weak_self.lock();
            if (!strong_self)
            {
                return;
            }

            if (ec)
            {
                if (ec.value() == operation_cancelled)
                {
                    spdlog::warn("session has been destroyed, return from "
                                 "async_read_some(), err = {}, msg = {}",
                        ec.value(), ec.message());
                    return;
                }

                spdlog::error("{} do_read() received error {}, msg = {}", id(), ec.value(), ec.message());

                if (ec != boost::asio::error::operation_aborted)
                {
                    session_manager_->stop(strong_self);
                    return;
                }
            }

            buffer_.socket_consume(bytes_transferred);

            // spdlog::debug("do_read() reads {} bytes, buffer has unread {} bytes",
            //               bytes_transferred, buffer_.unread_length());

            // if no error, then send to derived session class
            on_recv(buffer_);

            do_read();
        });
}

void session::do_write(const char *data, size_t size, bool is_async, bool is_close)
{
    if (!socket_.is_open())
    {
        spdlog::debug("{} socket has been closed, return from do_write()", id());
        return;
    }

    std::weak_ptr<session> weak_self = shared_from_this();
    if (is_async)
    {
        boost::asio::async_write(
            socket_, boost::asio::buffer(data, size), [this, is_close, weak_self](boost::system::error_code ec, std::size_t bytes_sent) {
                auto strong_self = weak_self.lock();
                if (!strong_self)
                {
                    return;
                }

                if (ec)
                {
                    spdlog::error("{} do_write() received error {}, msg = {}", id(), ec.value(), ec.message());

                    if (ec != boost::asio::error::operation_aborted)
                    {
                        session_manager_->stop(shared_from_this());
                        return;
                    }
                }
                else
                {
                    if (is_close)
                    {
                        session_manager_->stop(shared_from_this());
                    }
                }
            });
    }
    else
    {
        boost::system::error_code ec;
        size_t bytes_sent = boost::asio::write(socket_, boost::asio::buffer(data, size), ec);

        auto strong_self = weak_self.lock();
        if (!strong_self)
        {
            return;
        }

        if (ec)
        {
            spdlog::error("{} do_write() received error {}, msg = {}", id(), ec.value(), ec.message());

            if (ec != boost::asio::error::operation_aborted)
            {
                session_manager_->stop(strong_self);
                return;
            }
        }
        else
        {
            if (is_close)
            {
                session_manager_->stop(strong_self);
            }
        }
    }
}

void session::stop()
{
    if (socket_.is_open())
    {
        socket_.close();
        // Initiate graceful connection closure.
        // boost::system::error_code ignored_ec;
        // socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
        // ignored_ec);
    }
}

void session::shutdown()
{
    session_manager_->stop(shared_from_this());
}

// session_manager impl
session_manager::ptr session_manager::create(std::string prefix)
{
    return std::shared_ptr<session_manager>(new session_manager(std::move(prefix)));
}

session_manager::session_manager(std::string session_prefix)
    : session_prefix_{std::move(session_prefix)}
{}

std::string session_manager::generate_prefix()
{
    static std::atomic_int64_t session_index{0};
    return fmt::format("{}-session_count({})", session_prefix_, ++session_index);
}

void session_manager::add(const session::ptr &session_ptr)
{
    std::lock_guard<std::mutex> lock(mtx_);
    session_map_.emplace(session_ptr->id(), session_ptr);
}

void session_manager::stop(const session::ptr &session_ptr)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (session_ptr)
    {
        session_ptr->stop();
        session_map_.erase(session_ptr->id());
    }
}

void session_manager::stop_all()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (session_map_.empty())
    {
        return;
    }

    for (auto &it : session_map_)
    {
        if (it.second)
        {
            it.second->stop();
        }
    }

    session_map_.clear();
}

} // namespace network