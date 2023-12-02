
#include <spdlog/spdlog.h>

#include "network/tcp_server.h"

int main() {
#ifdef DEBUG
  spdlog::set_level(spdlog::level::debug); // Set global log level to debug
#endif

  boost::asio::io_context io_context;

  try {
    auto server = network::tcp_server::create(io_context, 8080);
    server->start();
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
    }
  }

  return EXIT_SUCCESS;
}