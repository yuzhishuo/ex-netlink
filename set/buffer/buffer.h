#ifndef LULUYUZHI_EXNETLINK_BUFFER_H
#define LULUYUZHI_EXNETLINK_BUFFER_H

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <algorithm>
#include <bitset>
#include <iostream>
#include <utility>

#include "yendian.h"

namespace luluyuzhi {

class circulation_buffer {
 public:
  constexpr static size_t padding = 4;
  explicit circulation_buffer(size_t length)
      : buf_{new uint8_t[length]{}},
        length_(length),
        readindex_{padding},
        writeindex_{padding} {}

  ~circulation_buffer() {
    delete[] buf_;
    length_ = 0;
    readindex_ = padding;
    writeindex_ = padding;
  }

  size_t readed_size() const {
    return writeindex_ < readindex_ ? length_ - readindex_ + writeindex_
                                    : writeindex_ - readindex_;
  }

  size_t writed_size() const {
    return writeindex_ <= readindex_ - padding
               ? readindex_ - padding - writeindex_
               : length_ - writeindex_ + readindex_ - padding;
  }

  inline void append(const char* data, size_t length) {
    append((const uint8_t*)data, length);
  }

  void append(const uint8_t* data, size_t length) {
    auto writeable_sz = writed_size();
    if (writeable_sz < length) {
      make_memory(length);
    }

    if (writeindex_ >= readindex_ && length_ - writeindex_ >= length) {
      std::copy_n(data, length, buf_ + writeindex_);
      writeindex_ += length;
    } else {
      if (writeindex_ < readindex_) {
        std::copy_n(data, length, buf_ + writeindex_);
        writeindex_ += length;
      } else {
        std::copy_n(data, length_ - writeindex_, buf_ + writeindex_);
        std::copy_n(data + length_ - writeindex_,
                    length - length_ + writeindex_, buf_);
        writeindex_ = length - length_ + writeindex_;
      }
    }
  }

  uint8_t* peek() { return buf_ + readindex_; }

  template <class T>
  T peek() {
    static_assert(std::is_standard_layout_v<T> && std::is_trivial_v<T>,
                  "require pod type");
    assert(readed_size() >= sizeof(T));
    T be16 = 0;

    if (writeindex_ < readindex_) {
      auto re_size = length_ - readindex_;
      if (re_size > sizeof(T)) {
        ::memcpy(&be16, peek(), sizeof(T));
      } else {
        T l{}, r{};
        ::memcpy(&l, peek(), re_size);
        ::memcpy(&r, buf_, sizeof(T) - re_size);
        be16 = l + (r << (re_size * 8));
      }
    } else {
      assert(writeindex_ - readindex_ >= sizeof(T));
      ::memcpy(&be16, peek(), sizeof(T));
    }

    return network2host<T>(be16);
  }

  void skip(size_t len) {
    assert(len <= readed_size());
    if (len < readed_size()) {
      if (writeindex_ < readindex_) {
        if (length_ - readindex_ > len) {
          readindex_ += len;
        } else {
          readindex_ = len - length_ + readindex_;
        }
      } else {
        readindex_ += len;
      }

    } else {
      retrieve_all();
    }
  }

  void retrieve_all() {
    readindex_ = padding;
    writeindex_ = padding;
  }

 private:
  void make_memory(size_t length) {
    if (length < writed_size()) {
      return;
    }
    relocation_location();
    size_t new_length = length_ + (length - writed_size()) * 2;
    uint8_t* new_buf = new uint8_t[new_length];
    std::copy_n(peek() - padding, writeindex_, new_buf);
    delete[] buf_;
    buf_ = new_buf;
    length_ = new_length;
  }

  void relocation_location() {
    if (writeindex_ < readindex_) {
      if (length_ - readindex_ > writeindex_) {
        uint8_t t[writeindex_];

        std::copy_n(buf_, writeindex_, t);
        std::copy_n(buf_ + readindex_ - padding, length_ - readindex_ + padding,
                    buf_);
        std::copy_n(t, writeindex_, buf_ + (length_ - readindex_ + padding));
        readindex_ = padding;
        writeindex_ = length_ - readindex_ + writeindex_;
      } else {
        uint8_t t[length_ - readindex_ + padding];

        std::copy_n(buf_ + readindex_ - padding, length_ - readindex_ + padding,
                    t);
        std::copy_n(buf_, writeindex_, buf_ + (length_ - readindex_ + padding));
        std::copy_n(t, length_ - readindex_ + padding, buf_);
        writeindex_ = length_ - readindex_ + writeindex_ + padding;
        readindex_ = padding;
      }
    }
  }

 private:
  uint8_t* buf_;
  size_t length_;
  size_t readindex_;
  size_t writeindex_;
};

};  // namespace luluyuzhi

#endif