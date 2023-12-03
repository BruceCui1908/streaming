
#include <spdlog/spdlog.h>

#include "network/tcp_server.h"
#include "protocol/rtmp/rtmp_session.h"

int main() {
#ifdef DEBUG
  spdlog::set_level(spdlog::level::debug); // Set global log level to debug
#endif

  boost::asio::io_context io_context;
  auto rtmpserver = network::tcp_server::create(io_context, 1935);

  try {
    rtmpserver->start<rtmp::rtmp_session>();
  } catch (std::exception &ex) {
    spdlog::error("Failed to initialize servers, error = {}", ex.what());
    return EXIT_FAILURE;
  }

  for (;;) {
    try {
      io_context.run();
      spdlog::info("io_context exited normally");
      break;
    } catch (std::exception &ex) {
      spdlog::error("io_context received exception, error = {}", ex.what());
      rtmpserver->restart();
    }
  }

  return EXIT_SUCCESS;
}