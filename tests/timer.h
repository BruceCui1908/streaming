#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <chrono>
#include <functional>
#include <iostream>

void test_timer1()
{
    std::cout << "Timer starts!" << std::endl;

    boost::asio::io_context io;
    boost::asio::steady_timer t(io, std::chrono::seconds(10));
    t.wait();

    std::cout << "Timer finished!" << std::endl;
}

void print(const boost::system::error_code &e)
{
    std::cout << "async timer finished" << std::endl;
}

void test_timer2()
{
    boost::asio::io_context io;

    boost::asio::steady_timer t(io, std::chrono::seconds(5));

    t.async_wait(&print);

    std::cout << "start io run" << std::endl;
    io.run();
    std::cout << "end iorun" << std::endl;
}

void print3(const boost::system::error_code &, boost::asio::steady_timer *t, int *count)
{
    if (*count < 5)
    {
        std::cout << *count << std::endl;
        ++(*count);

        t->expires_at(t->expiry() + std::chrono::seconds(1));
        t->async_wait(boost::bind(print3, boost::asio::placeholders::error, t, count));
    }
}

void test_timer3()
{
    boost::asio::io_context io;

    int count = 0;
    boost::asio::steady_timer t(io, std::chrono::seconds(1));
    t.async_wait(boost::bind(print3, boost::asio::placeholders::error, &t, &count));

    std::cout << "start io run" << std::endl;

    io.run();

    std::cout << "final count is " << count << std::endl;
}

void test_timer()
{
    test_timer3();
}