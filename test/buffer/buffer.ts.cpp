
#include <buffer.h>

#include <bitset>
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

  buff.skip(6);

  uint64_t t1 = 0;
  char tm[] = "56789043";
  ::memcpy(&t1, tm, sizeof(uint64_t));
  std::cout << std::bitset<sizeof(uint64_t) * 8>(t1) << std::endl;
  auto b2 = luluyuzhi::network2host<uint64_t>(t1);
  auto c3 = buff.peek<uint64_t>();

  assert(c3 == b2);

  buff.append("4321", 4);

  buff.skip(10);

  return 0;
}
