#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <utility>
class Bar {
 public:
  Bar(const char* str, size_t len) noexcept {
    data_ = reinterpret_cast<char*>(malloc(sizeof(char) * len));
    memcpy(data_, str, len);
    len_ = len;
    capacity_ = len;
  }

  Bar(const char* str) noexcept : Bar(str, strlen(str)) {}

  explicit Bar(size_t capacity) noexcept {
    data_ = reinterpret_cast<char*>(malloc(sizeof(char) * capacity));
    len_ = 0;
    capacity_ = capacity;
  }

  Bar(Bar& other) noexcept : Bar(other.data_, other.len_) {}

  Bar(Bar&& other) noexcept {
    data_ = other.data_;
    capacity_ = other.capacity_;
    len_ = other.len_;
  }

  ~Bar() {
    if (data_) {
      delete data_;
      data_ = nullptr;
    }
    capacity_ = 0;
    len_ = 0;
  }

  Bar& operator=(Bar& other) { copy_swap(other); }

  auto append(char c) noexcept -> void {
    if (len_ == capacity_) {
      this->copy_swap(std::move(Bar(this->capacity_ * 2, data_, len_)));
    }
    data_[++len_] = c;
  }

  auto append(const char* str, size_t len) noexcept -> void {
    if (len_ + len > capacity_) {
      this->copy_swap(std::move(Bar(this->capacity_ * 2 + len, data_, len_)));
    }
    memcpy(data_ + len_, str, len);
  }

  auto append(const char* str) noexcept -> void { append(str, strlen(str)); }

  auto append(const Bar& other) noexcept -> void {
    append(other.data_, other.len_);
  }

  auto append(const std::string& other) noexcept -> void {
    append(other.c_str(), other.length());
  }

  auto size() const noexcept { return len_; }

  auto capactiy() const noexcept { return capacity_; }

  auto data() noexcept  { return data_; }

 private:
  Bar(size_t capacity, const char* str, size_t len) noexcept : Bar(capacity) {
    memcpy(data_, str, len);
    len_ = len;
  }
  auto copy_swap(Bar other) noexcept -> void {
    this->data_ = other.data_;
    this->len_ = other.len_;
    this->capacity_ = other.capacity_;

    other.data_ = nullptr;
    other.len_ = 0;
    other.capacity_ = 0;
  }

 private:
  char* data_;
  size_t capacity_;
  size_t len_;
};