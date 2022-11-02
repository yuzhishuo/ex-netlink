#pragma once

#ifndef EXNETLINK_RTT_RTTSERVER
#define EXNETLINK_RTT_RTTSERVER

#include <stdio.h>

#include <algorithm>
#include <any>
#include <functional>

#include<buffer.h>
#include"rtt_common.h"

using onConnect = void (*)(std::any* user_data);

enum class TranslationState { UNKNOW, CONNECTTING, CONNECTED, STOP };

struct Translation {
  int fd_;
  TranslationState state_;
  luluyuzhi::circulation_buffer rbuffer = std::move(luluyuzhi::circulation_buffer{3 * sizeof(rtt_package)});
  luluyuzhi::circulation_buffer wbuffer = std::move(luluyuzhi::circulation_buffer{3 * sizeof(rtt_package)});
};

void translation_init(Translation& translation, int fd) {
  translation.fd_ = fd;
  translation.state_ = TranslationState::CONNECTED;
}

void translation_destory(Translation& translation) {
  translation.fd_ = -1;
  translation.rbuffer.retrieve_all();
  translation.wbuffer.retrieve_all();
}

#endif  // EXNETLINK_RTT_RTTSERVER