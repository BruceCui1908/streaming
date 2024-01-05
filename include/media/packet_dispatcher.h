#pragma once

#include "network/session.h"
#include "util/util.h"

#include <memory>
#include <list>
#include <unordered_map>
#include <set>
#include <mutex>
#include <algorithm>

namespace media {

template<typename T, typename = std::enable_if_t<util::is_shared_ptr_v<T>>>
class packet_dispatcher;

#define CLIENT_READER_PARAMS                                                                                                               \
    std::weak_ptr<network::session> weak_session, std::weak_ptr<packet_dispatcher<T>> weak_dispatcher, const std::string &token

/**
Every connected client must use this class to register after successful validation.
*/
template<typename T, typename = std::enable_if_t<util::is_shared_ptr_v<T>>>
class client_reader : public std::enable_shared_from_this<client_reader<T>>
{
public:
    using ptr = std::shared_ptr<client_reader>;

    friend class packet_dispatcher<T>;

    static ptr create(CLIENT_READER_PARAMS)
    {
        return std::shared_ptr<client_reader>(new client_reader(std::move(weak_session), std::move(weak_dispatcher), token));
    }

    bool is_alive() const
    {
        return (bool)weak_session_.lock();
    }

    void set_read_cb(std::function<void(const T &)> cb)
    {
        if (!cb)
        {
            throw std::runtime_error("cannot set empty read cb in client_reader");
        }

        read_cb_ = std::move(cb);
    }

    void on_read(const T &pkt)
    {
        if (!read_cb_)
        {
            detach();
            return;
        }

        read_cb_(pkt);
    }

    void set_detach(std::function<void(bool)> cb)
    {
        if (!cb)
        {
            throw std::runtime_error("cannot set empty detach cb in client_reader");
        }

        detach_cb_ = std::move(cb);
    }

    // The client actively shuts down the connection
    void leave()
    {

        spdlog::debug("{} is leaving", id_);

        if (!is_registered_)
        {
            return;
        }

        // release self from the packet_dispatcher
        if (auto strong_dispatcher_ = weak_dispatcher_.lock())
        {
            strong_dispatcher_->deregist_reader(this->shared_from_this());
        }
    }

    // Only when the broadcaster stops the session can it be considered normal; clients have no choice but to leave.
    void detach(bool is_normal = false)
    {
        if (is_registered_ && detach_cb_)
        {
            detach_cb_(is_normal);
        }
    }

    bool is_registered() const
    {
        return is_registered_;
    }

    const std::string &id() const
    {
        return id_;
    }

private:
    client_reader(CLIENT_READER_PARAMS)
        : weak_session_{std::move(weak_session)}
        , weak_dispatcher_{std::move(weak_dispatcher)}
    {
        auto strong_session = weak_session_.lock();
        if (!strong_session)
        {
            throw std::runtime_error("cannot create client_reader with invalid session");
        }

        id_ = fmt::format("Client[{}-{}]", strong_session->id(), token);
    }

    /// @brief  called by packet_dispatcher
    void set_is_registered()
    {
        is_registered_ = true;
    }

private:
    std::weak_ptr<network::session> weak_session_;
    std::weak_ptr<packet_dispatcher<T>> weak_dispatcher_;
    std::function<void(const T &)> read_cb_;
    std::function<void(bool)> detach_cb_;
    std::string id_;
    bool is_registered_{false};
};

/**
This class is used to store and distribute packets to clients; all clients register their callback functions here.
*/
template<typename T, typename>
class packet_dispatcher : public std::enable_shared_from_this<packet_dispatcher<T>>
{
public:
    using ptr = std::shared_ptr<packet_dispatcher>;
    using client_reader_ptr = typename client_reader<T>::ptr;

    static constexpr size_t kMaxPacketCacheSize = 1024;
    static constexpr size_t kMinPacketCacheSize = 32;

    static ptr create(size_t max_size = kMaxPacketCacheSize)
    {
        return std::shared_ptr<packet_dispatcher>(new packet_dispatcher(max_size));
    }

    ~packet_dispatcher()
    {
        detach_all_readers();
    }

    void regist_reader(client_reader_ptr client_ptr)
    {
        if (!client_ptr)
        {
            return;
        }

        // register in new clients set
        {
            std::scoped_lock lock(client_mtx_);
            client_ptr->set_is_registered();
            new_clients_.insert(std::move(client_ptr));
        }
    }

    void deregist_reader(client_reader_ptr client_ptr)
    {
        if (!client_ptr)
        {
            return;
        }

        {
            std::lock_guard<std::recursive_mutex> lock(client_mtx_);
            new_clients_.erase(client_ptr);
        }
    }

    void input(T &pkt)
    {
        // TODO
    }

    void detach_all_readers(bool is_normal = false)
    {
        if (new_clients_.empty() && old_clients_.empty())
        {
            return;
        }

        {
            std::lock_guard<std::recursive_mutex> lock(client_mtx_);
            for (auto it = new_clients_.begin(); it != new_clients_.end(); ++it)
            {
                auto &ptr = *it;
                if (ptr)
                {
                    ptr->detach();
                }
                it = new_clients_.erase(it);
            }

            for (auto it = old_clients_.begin(); it != old_clients_.end(); ++it)
            {
                auto &ptr = *it;
                if (ptr)
                {
                    ptr->detach();
                }
                it = old_clients_.erase(it);
            }
        }
    }

private:
    packet_dispatcher(size_t max_size)
    {
        if (max_size < kMinPacketCacheSize)
        {
            max_size = kMinPacketCacheSize;
        }
        max_size_ = max_size;
    }

private:
    // for packet list
    size_t max_size_;
    size_t packet_list_size_{0};
    std::recursive_mutex pkt_mtx_{};
    std::list<T> packet_list_{};

    // for readers set
    std::recursive_mutex client_mtx_{};
    std::set<client_reader_ptr> new_clients_{};
    std::set<client_reader_ptr> old_clients_{};
};

} // namespace media