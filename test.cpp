#include <bitset>
#include <iostream>

int main() {
  char a = 0;
  std::bitset<8> s(a);
  std::cout << s << std::endl;

  int id = 60;

  a |= id;
  std::bitset<8> s2(a);
  std::cout << s2 << std::endl;

  int num = 3;
  a |= num << 6;

  std::bitset<8> s3(a);
  std::cout << s3 << std::endl;
}