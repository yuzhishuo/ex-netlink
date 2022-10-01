
#include <buffer.h>

#include <iostream>
int main(int argc, char const *argv[]) {
  luluyuzhi::circulation_buffer buff(20);

  char buf[] = "1234567890";
  buff.append((const uint8_t *)buf, 10);
  auto c = buff.peek<uint8_t>();
  std::cout << c << std::endl;
  auto c1 = buff.peek<uint64_t>();

  assert(c1 == luluyuzhi::network2host<uint64_t>((*(uint64_t *)buf)));

  buff.skip(8);
  auto c2 = buff.peek<uint16_t>();

  uint16_t t = 0;
  ::memcpy(&t, buf + 8, sizeof(uint16_t));
  auto b1 = luluyuzhi::network2host<uint16_t>(t);
  assert(c2 == b1);

  buff.append("1234567890", 10);

  buff.append("4321", 4);
  buff.append("4321", 4);

  buff.skip(10);

  
  return 0;
}
