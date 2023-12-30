#include "network/tcp_server.h"
#include "protocol/rtmp/rtmp_session.h"
#include "protocol/http/http_session.h"
#include "util/singleton.h"

int main()
{
#ifdef DEBUG
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
#endif

    boost::asio::io_context io_context;
    auto rtmpserver = network::tcp_server::create(io_context, 1935);
    auto httpserver = network::tcp_server::create(io_context, 80);

    try
    {
        rtmpserver->start<rtmp::rtmp_session>();
        httpserver->start<http::http_session>();
    }
    catch (std::exception &ex)
    {
        spdlog::error("Failed to initialize servers, error = {}", ex.what());
        return EXIT_FAILURE;
    }

    auto &thread_pool = util::bs_thread_pool::instance();
    auto cpus = thread_pool.get_thread_count();

    for (auto i = 0; i < cpus; ++i)
    {
        thread_pool.detach_task([&]() {
            for (;;)
            {
                try
                {
                    io_context.run();
                    break;
                }
                catch (std::exception &ex)
                {
                    spdlog::error("io_context received exception, error = {}", ex.what());
                    rtmpserver->restart();
                    httpserver->restart();
                }
            }
        });
    }

    thread_pool.wait();

    spdlog::info("io_context exited normally");

    return EXIT_SUCCESS;
}