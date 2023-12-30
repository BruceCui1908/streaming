#pragma once

#include "BS_thread_pool.hpp"

template<typename T, typename = typename std::enable_if_t<std::is_default_constructible_v<T>>>
class singleton
{
public:
    using ptr = std::shared_ptr<singleton>;

    // static_assert(std::is_default_constructible_v<T>, "A class that needs to be built as a singleton must have a default constructor");

    ~singleton() = default;

    singleton(const singleton &) = delete;
    singleton &operator=(const singleton &) = delete;
    singleton(singleton &&) noexcept = delete;
    singleton &operator=(singleton &&) noexcept = delete;

    static T &instance()
    {
        static std::shared_ptr<singleton> ptr(new singleton);
        return ptr_->t;
    }

private:
    singleton() = default;

private:
    T t;
};

using bs_thread_pool = singleton<BS::thread_pool>;