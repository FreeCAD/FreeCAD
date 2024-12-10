// Formatting library for C++ - scanning API proof of concept
//
// Copyright (c) 2019 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <array>
#include <cassert>
#include <climits>

#include "fmt/format.h"

FMT_BEGIN_NAMESPACE
template <typename T, typename Char = char> struct scanner {
  // A deleted default constructor indicates a disabled scanner.
  scanner() = delete;
};

class scan_parse_context {
 private:
  string_view format_;

 public:
  using iterator = string_view::iterator;

  explicit FMT_CONSTEXPR scan_parse_context(string_view format)
      : format_(format) {}

  FMT_CONSTEXPR iterator begin() const { return format_.begin(); }
  FMT_CONSTEXPR iterator end() const { return format_.end(); }

  void advance_to(iterator it) {
    format_.remove_prefix(detail::to_unsigned(it - begin()));
  }
};

struct scan_context {
 private:
  string_view input_;

 public:
  using iterator = const char*;

  explicit scan_context(string_view input) : input_(input) {}

  iterator begin() const { return input_.data(); }
  iterator end() const { return begin() + input_.size(); }

  void advance_to(iterator it) {
    input_.remove_prefix(detail::to_unsigned(it - begin()));
  }
};

namespace detail {
enum class scan_type {
  none_type,
  int_type,
  uint_type,
  long_long_type,
  ulong_long_type,
  string_type,
  string_view_type,
  custom_type
};

struct custom_scan_arg {
  void* value;
  void (*scan)(void* arg, scan_parse_context& parse_ctx, scan_context& ctx);
};

class scan_arg {
 public:
  scan_type type;
  union {
    int* int_value;
    unsigned* uint_value;
    long long* long_long_value;
    unsigned long long* ulong_long_value;
    std::string* string;
    fmt::string_view* string_view;
    custom_scan_arg custom;
    // TODO: more types
  };

  scan_arg() : type(scan_type::none_type) {}
  scan_arg(int& value) : type(scan_type::int_type), int_value(&value) {}
  scan_arg(unsigned& value) : type(scan_type::uint_type), uint_value(&value) {}
  scan_arg(long long& value)
      : type(scan_type::long_long_type), long_long_value(&value) {}
  scan_arg(unsigned long long& value)
      : type(scan_type::ulong_long_type), ulong_long_value(&value) {}
  scan_arg(std::string& value) : type(scan_type::string_type), string(&value) {}
  scan_arg(fmt::string_view& value)
      : type(scan_type::string_view_type), string_view(&value) {}
  template <typename T> scan_arg(T& value) : type(scan_type::custom_type) {
    custom.value = &value;
    custom.scan = scan_custom_arg<T>;
  }

 private:
  template <typename T>
  static void scan_custom_arg(void* arg, scan_parse_context& parse_ctx,
                              scan_context& ctx) {
    scanner<T> s;
    parse_ctx.advance_to(s.parse(parse_ctx));
    ctx.advance_to(s.scan(*static_cast<T*>(arg), ctx));
  }
};
}  // namespace detail

struct scan_args {
  int size;
  const detail::scan_arg* data;

  template <size_t N>
  scan_args(const std::array<detail::scan_arg, N>& store)
      : size(N), data(store.data()) {
    static_assert(N < INT_MAX, "too many arguments");
  }
};

namespace detail {

struct scan_handler : error_handler {
 private:
  scan_parse_context parse_ctx_;
  scan_context scan_ctx_;
  scan_args args_;
  int next_arg_id_;
  scan_arg arg_;

  template <typename T = unsigned> T read_uint() {
    T value = 0;
    auto it = scan_ctx_.begin(), end = scan_ctx_.end();
    while (it != end) {
      char c = *it++;
      if (c < '0' || c > '9') on_error("invalid input");
      // TODO: check overflow
      value = value * 10 + static_cast<unsigned>(c - '0');
    }
    scan_ctx_.advance_to(it);
    return value;
  }

  template <typename T = int> T read_int() {
    auto it = scan_ctx_.begin(), end = scan_ctx_.end();
    bool negative = it != end && *it == '-';
    if (negative) ++it;
    scan_ctx_.advance_to(it);
    const auto value = read_uint<typename std::make_unsigned<T>::type>();
    if (negative) return -static_cast<T>(value);
    return static_cast<T>(value);
  }

 public:
  scan_handler(string_view format, string_view input, scan_args args)
      : parse_ctx_(format), scan_ctx_(input), args_(args), next_arg_id_(0) {}

  const char* pos() const { return scan_ctx_.begin(); }

  void on_text(const char* begin, const char* end) {
    auto size = to_unsigned(end - begin);
    auto it = scan_ctx_.begin();
    if (it + size > scan_ctx_.end() ||
        !std::equal(begin, end, make_checked(it, size))) {
      on_error("invalid input");
    }
    scan_ctx_.advance_to(it + size);
  }

  FMT_CONSTEXPR int on_arg_id() { return on_arg_id(next_arg_id_++); }
  FMT_CONSTEXPR int on_arg_id(int id) {
    if (id >= args_.size) on_error("argument index out of range");
    arg_ = args_.data[id];
    return id;
  }
  FMT_CONSTEXPR int on_arg_id(string_view id) {
    if (id.data()) on_error("invalid format");
    return 0;
  }

  void on_replacement_field(int, const char*) {
    auto it = scan_ctx_.begin(), end = scan_ctx_.end();
    switch (arg_.type) {
    case scan_type::int_type:
      *arg_.int_value = read_int();
      break;
    case scan_type::uint_type:
      *arg_.uint_value = read_uint();
      break;
    case scan_type::long_long_type:
      *arg_.long_long_value = read_int<long long>();
      break;
    case scan_type::ulong_long_type:
      *arg_.ulong_long_value = read_uint<unsigned long long>();
      break;
    case scan_type::string_type:
      while (it != end && *it != ' ') arg_.string->push_back(*it++);
      scan_ctx_.advance_to(it);
      break;
    case scan_type::string_view_type: {
      auto s = it;
      while (it != end && *it != ' ') ++it;
      *arg_.string_view = fmt::string_view(s, to_unsigned(it - s));
      scan_ctx_.advance_to(it);
      break;
    }
    case scan_type::none_type:
    case scan_type::custom_type:
      assert(false);
    }
  }

  const char* on_format_specs(int, const char* begin, const char*) {
    if (arg_.type != scan_type::custom_type) return begin;
    parse_ctx_.advance_to(begin);
    arg_.custom.scan(arg_.custom.value, parse_ctx_, scan_ctx_);
    return parse_ctx_.begin();
  }
};
}  // namespace detail

template <typename... Args>
std::array<detail::scan_arg, sizeof...(Args)> make_scan_args(Args&... args) {
  return {{args...}};
}

string_view::iterator vscan(string_view input, string_view format_str,
                            scan_args args) {
  detail::scan_handler h(format_str, input, args);
  detail::parse_format_string<false>(format_str, h);
  return input.begin() + (h.pos() - &*input.begin());
}

template <typename... Args>
string_view::iterator scan(string_view input, string_view format_str,
                           Args&... args) {
  return vscan(input, format_str, make_scan_args(args...));
}
FMT_END_NAMESPACE
