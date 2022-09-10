#ifndef EXNETLINK_RTT_RTTCOMMON_CPP
#define EXNETLINK_RTT_RTTCOMMON_CPP
#include <stdint.h>

static_assert(sizeof(int64_t) == 8, "rtt protocol require int64_t is 8 bytes.");

struct alignas(1) rtt_package {
  uint64_t c_send_ts = 0;
  uint64_t s_recv_ts = 0;
  uint64_t s_send_ts = 0;
};
static_assert(sizeof(rtt_package) == 24);

uint64_t timeval2ui64(struct timeval& val) {
  constexpr static auto unitdiff = 100'0000;
  return val.tv_sec * unitdiff + val.tv_usec;
}

#endif