// Formatting library for C++ - scanning API test
//
// Copyright (c) 2019 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "scan.h"

#include <time.h>

#include <climits>

#include "gmock/gmock.h"
#include "gtest-extra.h"

TEST(scan_test, read_text) {
  auto s = fmt::string_view("foo");
  auto end = fmt::scan(s, "foo");
  EXPECT_EQ(end, s.end());
  EXPECT_THROW_MSG(fmt::scan("fob", "foo"), fmt::format_error, "invalid input");
}

TEST(scan_test, read_int) {
  auto n = int();
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  fmt::scan("-42", "{}", n);
  EXPECT_EQ(n, -42);
}

TEST(scan_test, read_longlong) {
  long long n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  fmt::scan("-42", "{}", n);
  EXPECT_EQ(n, -42);
}

TEST(scan_test, read_uint) {
  auto n = unsigned();
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  EXPECT_THROW_MSG(fmt::scan("-42", "{}", n), fmt::format_error,
                   "invalid input");
}

TEST(scan_test, read_ulonglong) {
  unsigned long long n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  EXPECT_THROW_MSG(fmt::scan("-42", "{}", n), fmt::format_error,
                   "invalid input");
}

TEST(scan_test, read_string) {
  auto s = std::string();
  fmt::scan("foo", "{}", s);
  EXPECT_EQ(s, "foo");
}

TEST(scan_test, read_string_view) {
  auto s = fmt::string_view();
  fmt::scan("foo", "{}", s);
  EXPECT_EQ(s, "foo");
}

#ifndef _WIN32
namespace fmt {
template <> struct scanner<tm> {
  std::string format;

  scan_parse_context::iterator parse(scan_parse_context& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == ':') ++it;
    auto end = it;
    while (end != ctx.end() && *end != '}') ++end;
    format.reserve(detail::to_unsigned(end - it + 1));
    format.append(it, end);
    format.push_back('\0');
    return end;
  }

  template <class ScanContext>
  typename ScanContext::iterator scan(tm& t, ScanContext& ctx) {
    auto result = strptime(ctx.begin(), format.c_str(), &t);
    if (!result) throw format_error("failed to parse time");
    return result;
  }
};
}  // namespace fmt

TEST(scan_test, read_custom) {
  auto input = "Date: 1985-10-25";
  auto t = tm();
  fmt::scan(input, "Date: {0:%Y-%m-%d}", t);
  EXPECT_EQ(t.tm_year, 85);
  EXPECT_EQ(t.tm_mon, 9);
  EXPECT_EQ(t.tm_mday, 25);
}
#endif

TEST(scan_test, invalid_format) {
  EXPECT_THROW_MSG(fmt::scan("", "{}"), fmt::format_error,
                   "argument index out of range");
  EXPECT_THROW_MSG(fmt::scan("", "{"), fmt::format_error,
                   "invalid format string");
}

TEST(scan_test, example) {
  auto key = std::string();
  auto value = int();
  fmt::scan("answer = 42", "{} = {}", key, value);
  EXPECT_EQ(key, "answer");
  EXPECT_EQ(value, 42);
}
