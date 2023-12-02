
#include <spdlog/spdlog.h>

int main() {
#ifdef DEBUG
  spdlog::set_level(spdlog::level::debug); // Set global log level to debug
#endif

  return EXIT_SUCCESS;
}