#pragma once

#ifndef EXNETLINK_RTT_RTTSERVER
#define EXNETLINK_RTT_RTTSERVER

#include <stdio.h>

#include <algorithm>
#include <any>
#include <functional>

using onConnect = void (*)(std::any* user_data);

enum class TranslationState { UNKNOW, CONNECTTING, CONNECTED, STOP };

struct Translation {
  int fd_;
  TranslationState state_;
  std::any data_;
};

bool translation_init(Translation& translation, int fd) {
  translation = std::move(Translation{
      .fd_ = fd,
      .state_ = TranslationState::CONNECTED,
  });
}

template <typename T>
void translation_add_data(Translation& translation, T data) {
  translation.data_ = std::move(data);
}

template <typename T>
T* translation_get_data(Translation& translation) {
  return std::any_cast<T>(&translation.data_);
}

void translation_destory(Translation& translation) {
  translation = std::move(Translation{});
}

#endif  // EXNETLINK_RTT_RTTSERVER