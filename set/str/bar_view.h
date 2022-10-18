#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <cstdint>
#include <new>
#include <utility>
template <size_t len>
struct _palce {
  using type = uint8_t[len];
};

struct context {
  static constexpr auto locl_cache_len = sizeof(void*) == 8 ? 19 : 15;
  char* data;
  uint32_t len;
  union _ {
    uint32_t capacity_;
    char cache[locl_cache_len + 1];
  } inner;
#define contxt_capacity_ inner.capacity_
#define contxt_cache_ inner.cache

  context() = default;
};

static_assert(sizeof(context) == 32, "context size should equal 32");

class sso_str_ {
 public:
  sso_str_() { auto ptr = new (data) context; }

  inline auto get_context() {
    return reinterpret_cast<context*>(std::addressof(data));
  }

  ~sso_str_() { get_context()->~context(); }

 private:
  typename _palce<sizeof(context)>::type data;
};

class SsoString {
 public:
  explicit SsoString(const char* str, size_t len) noexcept {
    assert(strlen(str) == len);
    if (len <= str_.get_context()->locl_cache_len) {
      strcpy(str_.get_context()->contxt_cache_, str);
      str_.get_context()->contxt_cache_[len + 1] = '\0';
      return;
    }

    str_.get_context()->data = reinterpret_cast<char*>(malloc(len));
    str_.get_context()->len = len;
    str_.get_context()->contxt_capacity_ = len;
  }

  explicit SsoString(const char* str) noexcept : SsoString(str, strlen(str)) {}

 private:
  sso_str_ str_;
};