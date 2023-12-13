#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

std::weak_ptr<void> test1() { return nullptr; }

int main() {

  auto weak_obj = media_sources_["schema"]["vhost"]["app"]["stream"];

  std::cout << strong_obj << std::endl;
}
