// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

// Check if fmt/format.h compiles with windows.h included before it.
#ifdef _WIN32
#  include <windows.h>
#endif
// clang-format off
#include "fmt/format.h"
// clang-format on

#include <stdint.h>  // uint32_t

#include <climits>      // INT_MAX
#include <cmath>        // std::signbit
#include <cstring>      // std::strlen
#include <iterator>     // std::back_inserter
#include <list>         // std::list
#include <memory>       // std::unique_ptr
#include <type_traits>  // std::is_default_constructible

#include "gtest-extra.h"
#include "mock-allocator.h"
#include "util.h"

using fmt::basic_memory_buffer;
using fmt::format_error;
using fmt::memory_buffer;
using fmt::runtime;
using fmt::string_view;
using fmt::detail::max_value;
using fmt::detail::uint128_fallback;

using testing::Return;
using testing::StrictMock;

enum { buffer_size = 256 };

TEST(uint128_test, ctor) {
  auto n = uint128_fallback();
  EXPECT_EQ(n, 0);
  n = uint128_fallback(42);
  EXPECT_EQ(n, 42);
  EXPECT_EQ(static_cast<uint64_t>(n), 42);
}

TEST(uint128_test, shift) {
  auto n = uint128_fallback(42);
  n = n << 64;
  EXPECT_EQ(static_cast<uint64_t>(n), 0);
  n = n >> 64;
  EXPECT_EQ(static_cast<uint64_t>(n), 42);
  n = n << 62;
  EXPECT_EQ(static_cast<uint64_t>(n >> 64), 0xa);
  EXPECT_EQ(static_cast<uint64_t>(n), 0x8000000000000000);
  n = n >> 62;
  EXPECT_EQ(static_cast<uint64_t>(n), 42);
  EXPECT_EQ(uint128_fallback(1) << 112, uint128_fallback(0x1000000000000, 0));
  EXPECT_EQ(uint128_fallback(0x1000000000000, 0) >> 112, uint128_fallback(1));
}

TEST(uint128_test, minus) {
  auto n = uint128_fallback(42);
  EXPECT_EQ(n - 2, 40);
}

TEST(uint128_test, plus_assign) {
  auto n = uint128_fallback(32);
  n += uint128_fallback(10);
  EXPECT_EQ(n, 42);
  n = uint128_fallback(max_value<uint64_t>());
  n += uint128_fallback(1);
  EXPECT_EQ(n, uint128_fallback(1) << 64);
}

TEST(uint128_test, multiply) {
  auto n = uint128_fallback(2251799813685247);
  n = n * 3611864890;
  EXPECT_EQ(static_cast<uint64_t>(n >> 64), 440901);
}

template <typename Float> void check_isfinite() {
  using fmt::detail::isfinite;
  EXPECT_TRUE(isfinite(Float(0.0)));
  EXPECT_TRUE(isfinite(Float(42.0)));
  EXPECT_TRUE(isfinite(Float(-42.0)));
  EXPECT_TRUE(isfinite(Float(fmt::detail::max_value<double>())));
  // Use double because std::numeric_limits is broken for __float128.
  using limits = std::numeric_limits<double>;
  FMT_CONSTEXPR20 auto result = isfinite(Float(limits::infinity()));
  EXPECT_FALSE(result);
  EXPECT_FALSE(isfinite(Float(limits::infinity())));
  EXPECT_FALSE(isfinite(Float(-limits::infinity())));
  EXPECT_FALSE(isfinite(Float(limits::quiet_NaN())));
  EXPECT_FALSE(isfinite(Float(-limits::quiet_NaN())));
}

TEST(float_test, isfinite) {
  check_isfinite<double>();
#if FMT_USE_FLOAT128
  check_isfinite<fmt::detail::float128>();
#endif
}

template <typename Float> void check_isnan() {
  using fmt::detail::isnan;
  EXPECT_FALSE(isnan(Float(0.0)));
  EXPECT_FALSE(isnan(Float(42.0)));
  EXPECT_FALSE(isnan(Float(-42.0)));
  EXPECT_FALSE(isnan(Float(fmt::detail::max_value<double>())));
  // Use double because std::numeric_limits is broken for __float128.
  using limits = std::numeric_limits<double>;
  EXPECT_FALSE(isnan(Float(limits::infinity())));
  EXPECT_FALSE(isnan(Float(-limits::infinity())));
  EXPECT_TRUE(isnan(Float(limits::quiet_NaN())));
  EXPECT_TRUE(isnan(Float(-limits::quiet_NaN())));
}

TEST(float_test, isnan) {
  check_isnan<double>();
#if FMT_USE_FLOAT128
  check_isnan<fmt::detail::float128>();
#endif
}

struct uint32_pair {
  uint32_t u[2];
};

TEST(util_test, bit_cast) {
  auto s = fmt::detail::bit_cast<uint32_pair>(uint64_t{42});
  EXPECT_EQ(fmt::detail::bit_cast<uint64_t>(s), 42ull);
  s = fmt::detail::bit_cast<uint32_pair>(~uint64_t{0});
  EXPECT_EQ(fmt::detail::bit_cast<uint64_t>(s), ~0ull);
}

// Increment a number in a string.
void increment(char* s) {
  for (int i = static_cast<int>(std::strlen(s)) - 1; i >= 0; --i) {
    if (s[i] != '9') {
      ++s[i];
      break;
    }
    s[i] = '0';
  }
}

TEST(util_test, increment) {
  char s[10] = "123";
  increment(s);
  EXPECT_STREQ("124", s);
  s[2] = '8';
  increment(s);
  EXPECT_STREQ("129", s);
  increment(s);
  EXPECT_STREQ("130", s);
  s[1] = s[2] = '9';
  increment(s);
  EXPECT_STREQ("200", s);
}

TEST(util_test, parse_nonnegative_int) {
  auto s = fmt::string_view("10000000000");
  auto begin = s.begin(), end = s.end();
  EXPECT_EQ(fmt::detail::parse_nonnegative_int(begin, end, -1), -1);
  s = "2147483649";
  begin = s.begin();
  end = s.end();
  EXPECT_EQ(fmt::detail::parse_nonnegative_int(begin, end, -1), -1);
}

TEST(util_test, utf8_to_utf16) {
  auto u = fmt::detail::utf8_to_utf16("лошадка");
  EXPECT_EQ(L"\x043B\x043E\x0448\x0430\x0434\x043A\x0430", u.str());
  EXPECT_EQ(7, u.size());
  // U+10437 { DESERET SMALL LETTER YEE }
  EXPECT_EQ(L"\xD801\xDC37", fmt::detail::utf8_to_utf16("𐐷").str());
  EXPECT_THROW_MSG(fmt::detail::utf8_to_utf16("\xc3\x28"), std::runtime_error,
                   "invalid utf8");
  EXPECT_THROW_MSG(fmt::detail::utf8_to_utf16(fmt::string_view("л", 1)),
                   std::runtime_error, "invalid utf8");
  EXPECT_EQ(L"123456", fmt::detail::utf8_to_utf16("123456").str());
}

TEST(util_test, utf8_to_utf16_empty_string) {
  auto s = std::string();
  auto u = fmt::detail::utf8_to_utf16(s.c_str());
  EXPECT_EQ(L"", u.str());
  EXPECT_EQ(s.size(), u.size());
}

TEST(util_test, allocator_ref) {
  using test_allocator_ref = allocator_ref<mock_allocator<int>>;
  auto check_forwarding = [](mock_allocator<int>& alloc,
                             test_allocator_ref& ref) {
    int mem;
    // Check if value_type is properly defined.
    allocator_ref<mock_allocator<int>>::value_type* ptr = &mem;
    // Check forwarding.
    EXPECT_CALL(alloc, allocate(42)).WillOnce(Return(ptr));
    ref.allocate(42);
    EXPECT_CALL(alloc, deallocate(ptr, 42));
    ref.deallocate(ptr, 42);
  };

  StrictMock<mock_allocator<int>> alloc;
  auto ref = test_allocator_ref(&alloc);
  // Check if allocator_ref forwards to the underlying allocator.
  check_forwarding(alloc, ref);
  test_allocator_ref ref2(ref);
  check_forwarding(alloc, ref2);
  test_allocator_ref ref3;
  EXPECT_EQ(nullptr, ref3.get());
  ref3 = ref;
  check_forwarding(alloc, ref3);
}

TEST(util_test, format_system_error) {
  fmt::memory_buffer message;
  fmt::format_system_error(message, EDOM, "test");
  auto ec = std::error_code(EDOM, std::generic_category());
  EXPECT_EQ(to_string(message), std::system_error(ec, "test").what());
  message = fmt::memory_buffer();

  // Check if std::allocator throws on allocating max size_t / 2 chars.
  size_t max_size = max_value<size_t>() / 2;
  bool throws_on_alloc = false;
  try {
    auto alloc = std::allocator<char>();
    alloc.deallocate(alloc.allocate(max_size), max_size);
  } catch (const std::bad_alloc&) {
    throws_on_alloc = true;
  }
  if (!throws_on_alloc) {
    fmt::print("warning: std::allocator allocates {} chars\n", max_size);
    return;
  }
}

TEST(util_test, system_error) {
  auto test_error = fmt::system_error(EDOM, "test");
  auto ec = std::error_code(EDOM, std::generic_category());
  EXPECT_STREQ(test_error.what(), std::system_error(ec, "test").what());
  EXPECT_EQ(test_error.code(), ec);

  auto error = std::system_error(std::error_code());
  try {
    throw fmt::system_error(EDOM, "test {}", "error");
  } catch (const std::system_error& e) {
    error = e;
  }
  fmt::memory_buffer message;
  fmt::format_system_error(message, EDOM, "test error");
  EXPECT_EQ(error.what(), to_string(message));
  EXPECT_EQ(error.code(), std::error_code(EDOM, std::generic_category()));
}

TEST(util_test, report_system_error) {
  fmt::memory_buffer out;
  fmt::format_system_error(out, EDOM, "test error");
  out.push_back('\n');
  EXPECT_WRITE(stderr, fmt::report_system_error(EDOM, "test error"),
               to_string(out));
}

TEST(memory_buffer_test, ctor) {
  basic_memory_buffer<char, 123> buffer;
  EXPECT_EQ(static_cast<size_t>(0), buffer.size());
  EXPECT_EQ(123u, buffer.capacity());
}

using std_allocator = allocator_ref<std::allocator<char>>;

TEST(memory_buffer_test, move_ctor_inline_buffer) {
  auto check_move_buffer =
      [](const char* str, basic_memory_buffer<char, 5, std_allocator>& buffer) {
        std::allocator<char>* alloc = buffer.get_allocator().get();
        basic_memory_buffer<char, 5, std_allocator> buffer2(std::move(buffer));
        // Move shouldn't destroy the inline content of the first buffer.
        EXPECT_EQ(str, std::string(&buffer[0], buffer.size()));
        EXPECT_EQ(str, std::string(&buffer2[0], buffer2.size()));
        EXPECT_EQ(5u, buffer2.capacity());
        // Move should transfer allocator.
        EXPECT_EQ(nullptr, buffer.get_allocator().get());
        EXPECT_EQ(alloc, buffer2.get_allocator().get());
      };

  auto alloc = std::allocator<char>();
  basic_memory_buffer<char, 5, std_allocator> buffer((std_allocator(&alloc)));
  const char test[] = "test";
  buffer.append(string_view(test, 4));
  check_move_buffer("test", buffer);
  // Adding one more character fills the inline buffer, but doesn't cause
  // dynamic allocation.
  buffer.push_back('a');
  check_move_buffer("testa", buffer);
}

TEST(memory_buffer_test, move_ctor_dynamic_buffer) {
  auto alloc = std::allocator<char>();
  basic_memory_buffer<char, 4, std_allocator> buffer((std_allocator(&alloc)));
  const char test[] = "test";
  buffer.append(test, test + 4);
  const char* inline_buffer_ptr = &buffer[0];
  // Adding one more character causes the content to move from the inline to
  // a dynamically allocated buffer.
  buffer.push_back('a');
  basic_memory_buffer<char, 4, std_allocator> buffer2(std::move(buffer));
  // Move should rip the guts of the first buffer.
  EXPECT_EQ(&buffer[0], inline_buffer_ptr);
  EXPECT_EQ(buffer.size(), 0);
  EXPECT_EQ(std::string(&buffer2[0], buffer2.size()), "testa");
  EXPECT_GT(buffer2.capacity(), 4u);
}

void check_move_assign_buffer(const char* str,
                              basic_memory_buffer<char, 5>& buffer) {
  basic_memory_buffer<char, 5> buffer2;
  buffer2 = std::move(buffer);
  // Move shouldn't destroy the inline content of the first buffer.
  EXPECT_EQ(str, std::string(&buffer[0], buffer.size()));
  EXPECT_EQ(str, std::string(&buffer2[0], buffer2.size()));
  EXPECT_EQ(5u, buffer2.capacity());
}

TEST(memory_buffer_test, move_assignment) {
  basic_memory_buffer<char, 5> buffer;
  const char test[] = "test";
  buffer.append(test, test + 4);
  check_move_assign_buffer("test", buffer);
  // Adding one more character fills the inline buffer, but doesn't cause
  // dynamic allocation.
  buffer.push_back('a');
  check_move_assign_buffer("testa", buffer);
  const char* inline_buffer_ptr = &buffer[0];
  // Adding one more character causes the content to move from the inline to
  // a dynamically allocated buffer.
  buffer.push_back('b');
  basic_memory_buffer<char, 5> buffer2;
  buffer2 = std::move(buffer);
  // Move should rip the guts of the first buffer.
  EXPECT_EQ(inline_buffer_ptr, &buffer[0]);
  EXPECT_EQ("testab", std::string(&buffer2[0], buffer2.size()));
  EXPECT_GT(buffer2.capacity(), 5u);
}

TEST(memory_buffer_test, grow) {
  typedef allocator_ref<mock_allocator<int>> Allocator;
  mock_allocator<int> alloc;
  basic_memory_buffer<int, 10, Allocator> buffer((Allocator(&alloc)));
  buffer.resize(7);
  using fmt::detail::to_unsigned;
  for (int i = 0; i < 7; ++i) buffer[to_unsigned(i)] = i * i;
  EXPECT_EQ(10u, buffer.capacity());
  int mem[20];
  mem[7] = 0xdead;
  EXPECT_CALL(alloc, allocate(20)).WillOnce(Return(mem));
  buffer.try_reserve(20);
  EXPECT_EQ(20u, buffer.capacity());
  // Check if size elements have been copied
  for (int i = 0; i < 7; ++i) EXPECT_EQ(i * i, buffer[to_unsigned(i)]);
  // and no more than that.
  EXPECT_EQ(0xdead, buffer[7]);
  EXPECT_CALL(alloc, deallocate(mem, 20));
}

TEST(memory_buffer_test, allocator) {
  using test_allocator = allocator_ref<mock_allocator<char>>;
  basic_memory_buffer<char, 10, test_allocator> buffer;
  EXPECT_EQ(nullptr, buffer.get_allocator().get());
  StrictMock<mock_allocator<char>> alloc;
  char mem;
  {
    basic_memory_buffer<char, 10, test_allocator> buffer2(
        (test_allocator(&alloc)));
    EXPECT_EQ(&alloc, buffer2.get_allocator().get());
    size_t size = 2 * fmt::inline_buffer_size;
    EXPECT_CALL(alloc, allocate(size)).WillOnce(Return(&mem));
    buffer2.reserve(size);
    EXPECT_CALL(alloc, deallocate(&mem, size));
  }
}

TEST(memory_buffer_test, exception_in_deallocate) {
  using test_allocator = allocator_ref<mock_allocator<char>>;
  StrictMock<mock_allocator<char>> alloc;
  basic_memory_buffer<char, 10, test_allocator> buffer(
      (test_allocator(&alloc)));
  size_t size = 2 * fmt::inline_buffer_size;
  auto mem = std::vector<char>(size);
  {
    EXPECT_CALL(alloc, allocate(size)).WillOnce(Return(&mem[0]));
    buffer.resize(size);
    std::fill(&buffer[0], &buffer[0] + size, 'x');
  }
  auto mem2 = std::vector<char>(2 * size);
  {
    EXPECT_CALL(alloc, allocate(2 * size)).WillOnce(Return(&mem2[0]));
    auto e = std::exception();
    EXPECT_CALL(alloc, deallocate(&mem[0], size)).WillOnce(testing::Throw(e));
    EXPECT_THROW(buffer.reserve(2 * size), std::exception);
    EXPECT_EQ(&mem2[0], &buffer[0]);
    // Check that the data has been copied.
    for (size_t i = 0; i < size; ++i) EXPECT_EQ('x', buffer[i]);
  }
  EXPECT_CALL(alloc, deallocate(&mem2[0], 2 * size));
}

template <typename Allocator, size_t MaxSize>
class max_size_allocator : public Allocator {
 public:
  using typename Allocator::value_type;
  size_t max_size() const noexcept { return MaxSize; }
  value_type* allocate(size_t n) {
    if (n > max_size()) {
      throw std::length_error("size > max_size");
    }
    return std::allocator_traits<Allocator>::allocate(
        *static_cast<Allocator*>(this), n);
  }
  void deallocate(value_type* p, size_t n) {
    std::allocator_traits<Allocator>::deallocate(*static_cast<Allocator*>(this),
                                                 p, n);
  }
};

TEST(memory_buffer_test, max_size_allocator) {
  // 160 = 128 + 32
  using test_allocator = max_size_allocator<std::allocator<char>, 160>;
  basic_memory_buffer<char, 10, test_allocator> buffer;
  buffer.resize(128);
  // new_capacity = 128 + 128/2 = 192 > 160
  buffer.resize(160);  // Shouldn't throw.
}

TEST(memory_buffer_test, max_size_allocator_overflow) {
  using test_allocator = max_size_allocator<std::allocator<char>, 160>;
  basic_memory_buffer<char, 10, test_allocator> buffer;
  EXPECT_THROW(buffer.resize(161), std::exception);
}

TEST(format_test, escape) {
  EXPECT_EQ("{", fmt::format("{{"));
  EXPECT_EQ("before {", fmt::format("before {{"));
  EXPECT_EQ("{ after", fmt::format("{{ after"));
  EXPECT_EQ("before { after", fmt::format("before {{ after"));

  EXPECT_EQ("}", fmt::format("}}"));
  EXPECT_EQ("before }", fmt::format("before }}"));
  EXPECT_EQ("} after", fmt::format("}} after"));
  EXPECT_EQ("before } after", fmt::format("before }} after"));

  EXPECT_EQ("{}", fmt::format("{{}}"));
  EXPECT_EQ("{42}", fmt::format("{{{0}}}", 42));
}

TEST(format_test, unmatched_braces) {
  EXPECT_THROW_MSG((void)fmt::format(runtime("{")), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("}")), format_error,
                   "unmatched '}' in format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0{}")), format_error,
                   "invalid format string");
}

TEST(format_test, no_args) { EXPECT_EQ("test", fmt::format("test")); }

TEST(format_test, args_in_different_positions) {
  EXPECT_EQ("42", fmt::format("{0}", 42));
  EXPECT_EQ("before 42", fmt::format("before {0}", 42));
  EXPECT_EQ("42 after", fmt::format("{0} after", 42));
  EXPECT_EQ("before 42 after", fmt::format("before {0} after", 42));
  EXPECT_EQ("answer = 42", fmt::format("{0} = {1}", "answer", 42));
  EXPECT_EQ("42 is the answer", fmt::format("{1} is the {0}", "answer", 42));
  EXPECT_EQ("abracadabra", fmt::format("{0}{1}{0}", "abra", "cad"));
}

TEST(format_test, arg_errors) {
  EXPECT_THROW_MSG((void)fmt::format(runtime("{")), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{?}")), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0")), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0}")), format_error,
                   "argument not found");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{00}"), 42), format_error,
                   "invalid format string");

  char format_str[buffer_size];
  safe_sprintf(format_str, "{%u", INT_MAX);
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str)), format_error,
                   "invalid format string");
  safe_sprintf(format_str, "{%u}", INT_MAX);
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str)), format_error,
                   "argument not found");

  safe_sprintf(format_str, "{%u", INT_MAX + 1u);
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str)), format_error,
                   "invalid format string");
  safe_sprintf(format_str, "{%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str)), format_error,
                   "argument not found");
}

template <int N> struct test_format {
  template <typename... T>
  static std::string format(fmt::string_view fmt, const T&... args) {
    return test_format<N - 1>::format(fmt, N - 1, args...);
  }
};

template <> struct test_format<0> {
  template <typename... T>
  static std::string format(fmt::string_view fmt, const T&... args) {
    return fmt::format(runtime(fmt), args...);
  }
};

TEST(format_test, many_args) {
  EXPECT_EQ("19", test_format<20>::format("{19}"));
  EXPECT_THROW_MSG(test_format<20>::format("{20}"), format_error,
                   "argument not found");
  EXPECT_THROW_MSG(test_format<21>::format("{21}"), format_error,
                   "argument not found");
  using fmt::detail::max_packed_args;
  std::string format_str = fmt::format("{{{}}}", max_packed_args + 1);
  EXPECT_THROW_MSG(test_format<max_packed_args>::format(format_str),
                   format_error, "argument not found");
}

TEST(format_test, named_arg) {
  EXPECT_EQ("1/a/A", fmt::format("{_1}/{a_}/{A_}", fmt::arg("a_", 'a'),
                                 fmt::arg("A_", "A"), fmt::arg("_1", 1)));
  EXPECT_EQ(" -42", fmt::format("{0:{width}}", -42, fmt::arg("width", 4)));
  EXPECT_EQ("st",
            fmt::format("{0:.{precision}}", "str", fmt::arg("precision", 2)));
  EXPECT_EQ("1 2", fmt::format("{} {two}", 1, fmt::arg("two", 2)));
  EXPECT_EQ("42",
            fmt::format("{c}", fmt::arg("a", 0), fmt::arg("b", 0),
                        fmt::arg("c", 42), fmt::arg("d", 0), fmt::arg("e", 0),
                        fmt::arg("f", 0), fmt::arg("g", 0), fmt::arg("h", 0),
                        fmt::arg("i", 0), fmt::arg("j", 0), fmt::arg("k", 0),
                        fmt::arg("l", 0), fmt::arg("m", 0), fmt::arg("n", 0),
                        fmt::arg("o", 0), fmt::arg("p", 0)));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{a}")), format_error,
                   "argument not found");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{a}"), 42), format_error,
                   "argument not found");
}

TEST(format_test, auto_arg_index) {
  EXPECT_EQ("abc", fmt::format("{}{}{}", 'a', 'b', 'c'));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0}{}"), 'a', 'b'), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{}{0}"), 'a', 'b'), format_error,
                   "cannot switch from automatic to manual argument indexing");
  EXPECT_EQ("1.2", fmt::format("{:.{}}", 1.2345, 2));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0}:.{}"), 1.2345, 2),
                   format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:.{0}}"), 1.2345, 2),
                   format_error,
                   "cannot switch from automatic to manual argument indexing");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{}")), format_error,
                   "argument not found");
}

TEST(format_test, empty_specs) { EXPECT_EQ("42", fmt::format("{0:}", 42)); }

TEST(format_test, left_align) {
  EXPECT_EQ("42  ", fmt::format("{0:<4}", 42));
  EXPECT_EQ("42  ", fmt::format("{0:<4o}", 042));
  EXPECT_EQ("42  ", fmt::format("{0:<4x}", 0x42));
  EXPECT_EQ("-42  ", fmt::format("{0:<5}", -42));
  EXPECT_EQ("42   ", fmt::format("{0:<5}", 42u));
  EXPECT_EQ("-42  ", fmt::format("{0:<5}", -42l));
  EXPECT_EQ("42   ", fmt::format("{0:<5}", 42ul));
  EXPECT_EQ("-42  ", fmt::format("{0:<5}", -42ll));
  EXPECT_EQ("42   ", fmt::format("{0:<5}", 42ull));
  EXPECT_EQ("-42  ", fmt::format("{0:<5}", -42.0));
  EXPECT_EQ("-42  ", fmt::format("{0:<5}", -42.0l));
  EXPECT_EQ("c    ", fmt::format("{0:<5}", 'c'));
  EXPECT_EQ("abc  ", fmt::format("{0:<5}", "abc"));
  EXPECT_EQ("0xface  ", fmt::format("{0:<8}", reinterpret_cast<void*>(0xface)));
}

TEST(format_test, right_align) {
  EXPECT_EQ("  42", fmt::format("{0:>4}", 42));
  EXPECT_EQ("  42", fmt::format("{0:>4o}", 042));
  EXPECT_EQ("  42", fmt::format("{0:>4x}", 0x42));
  EXPECT_EQ("  -42", fmt::format("{0:>5}", -42));
  EXPECT_EQ("   42", fmt::format("{0:>5}", 42u));
  EXPECT_EQ("  -42", fmt::format("{0:>5}", -42l));
  EXPECT_EQ("   42", fmt::format("{0:>5}", 42ul));
  EXPECT_EQ("  -42", fmt::format("{0:>5}", -42ll));
  EXPECT_EQ("   42", fmt::format("{0:>5}", 42ull));
  EXPECT_EQ("  -42", fmt::format("{0:>5}", -42.0));
  EXPECT_EQ("  -42", fmt::format("{0:>5}", -42.0l));
  EXPECT_EQ("    c", fmt::format("{0:>5}", 'c'));
  EXPECT_EQ("  abc", fmt::format("{0:>5}", "abc"));
  EXPECT_EQ("  0xface", fmt::format("{0:>8}", reinterpret_cast<void*>(0xface)));
}

TEST(format_test, center_align) {
  EXPECT_EQ(" 42  ", fmt::format("{0:^5}", 42));
  EXPECT_EQ(" 42  ", fmt::format("{0:^5o}", 042));
  EXPECT_EQ(" 42  ", fmt::format("{0:^5x}", 0x42));
  EXPECT_EQ(" -42 ", fmt::format("{0:^5}", -42));
  EXPECT_EQ(" 42  ", fmt::format("{0:^5}", 42u));
  EXPECT_EQ(" -42 ", fmt::format("{0:^5}", -42l));
  EXPECT_EQ(" 42  ", fmt::format("{0:^5}", 42ul));
  EXPECT_EQ(" -42 ", fmt::format("{0:^5}", -42ll));
  EXPECT_EQ(" 42  ", fmt::format("{0:^5}", 42ull));
  EXPECT_EQ(" -42 ", fmt::format("{0:^5}", -42.0));
  EXPECT_EQ(" -42 ", fmt::format("{0:^5}", -42.0l));
  EXPECT_EQ("  c  ", fmt::format("{0:^5}", 'c'));
  EXPECT_EQ(" abc  ", fmt::format("{0:^6}", "abc"));
  EXPECT_EQ(" 0xface ", fmt::format("{0:^8}", reinterpret_cast<void*>(0xface)));
}

TEST(format_test, fill) {
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{<5}"), 'c'), format_error,
                   "invalid fill character '{'");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{<5}}"), 'c'), format_error,
                   "invalid fill character '{'");
  EXPECT_EQ("**42", fmt::format("{0:*>4}", 42));
  EXPECT_EQ("**-42", fmt::format("{0:*>5}", -42));
  EXPECT_EQ("***42", fmt::format("{0:*>5}", 42u));
  EXPECT_EQ("**-42", fmt::format("{0:*>5}", -42l));
  EXPECT_EQ("***42", fmt::format("{0:*>5}", 42ul));
  EXPECT_EQ("**-42", fmt::format("{0:*>5}", -42ll));
  EXPECT_EQ("***42", fmt::format("{0:*>5}", 42ull));
  EXPECT_EQ("**-42", fmt::format("{0:*>5}", -42.0));
  EXPECT_EQ("**-42", fmt::format("{0:*>5}", -42.0l));
  EXPECT_EQ("c****", fmt::format("{0:*<5}", 'c'));
  EXPECT_EQ("abc**", fmt::format("{0:*<5}", "abc"));
  EXPECT_EQ("**0xface",
            fmt::format("{0:*>8}", reinterpret_cast<void*>(0xface)));
  EXPECT_EQ("foo=", fmt::format("{:}=", "foo"));
  EXPECT_EQ(std::string("\0\0\0*", 4),
            fmt::format(string_view("{:\0>4}", 6), '*'));
  EXPECT_EQ("жж42", fmt::format("{0:ж>4}", 42));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:\x80\x80\x80\x80\x80>}"), 0),
                   format_error, "invalid type specifier");
}

TEST(format_test, plus_sign) {
  EXPECT_EQ("+42", fmt::format("{0:+}", 42));
  EXPECT_EQ("-42", fmt::format("{0:+}", -42));
  EXPECT_EQ("+42", fmt::format("{0:+}", 42));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:+}"), 42u), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ("+42", fmt::format("{0:+}", 42l));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:+}"), 42ul), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ("+42", fmt::format("{0:+}", 42ll));
#if FMT_USE_INT128
  EXPECT_EQ("+42", fmt::format("{0:+}", __int128_t(42)));
#endif
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:+}"), 42ull), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ("+42", fmt::format("{0:+}", 42.0));
  EXPECT_EQ("+42", fmt::format("{0:+}", 42.0l));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:+"), 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:+}"), 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:+}"), "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(
      (void)fmt::format(runtime("{0:+}"), reinterpret_cast<void*>(0x42)),
      format_error, "format specifier requires numeric argument");
}

TEST(format_test, minus_sign) {
  EXPECT_EQ("42", fmt::format("{0:-}", 42));
  EXPECT_EQ("-42", fmt::format("{0:-}", -42));
  EXPECT_EQ("42", fmt::format("{0:-}", 42));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:-}"), 42u), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ("42", fmt::format("{0:-}", 42l));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:-}"), 42ul), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ("42", fmt::format("{0:-}", 42ll));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:-}"), 42ull), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ("42", fmt::format("{0:-}", 42.0));
  EXPECT_EQ("42", fmt::format("{0:-}", 42.0l));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:-"), 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:-}"), 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:-}"), "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(
      (void)fmt::format(runtime("{0:-}"), reinterpret_cast<void*>(0x42)),
      format_error, "format specifier requires numeric argument");
}

TEST(format_test, space_sign) {
  EXPECT_EQ(" 42", fmt::format("{0: }", 42));
  EXPECT_EQ("-42", fmt::format("{0: }", -42));
  EXPECT_EQ(" 42", fmt::format("{0: }", 42));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0: }"), 42u), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ(" 42", fmt::format("{0: }", 42l));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0: }"), 42ul), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ(" 42", fmt::format("{0: }", 42ll));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0: }"), 42ull), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ(" 42", fmt::format("{0: }", 42.0));
  EXPECT_EQ(" 42", fmt::format("{0: }", 42.0l));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0: "), 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0: }"), 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0: }"), "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(
      (void)fmt::format(runtime("{0: }"), reinterpret_cast<void*>(0x42)),
      format_error, "format specifier requires numeric argument");
}

TEST(format_test, hash_flag) {
  EXPECT_EQ("42", fmt::format("{0:#}", 42));
  EXPECT_EQ("-42", fmt::format("{0:#}", -42));
  EXPECT_EQ("0b101010", fmt::format("{0:#b}", 42));
  EXPECT_EQ("0B101010", fmt::format("{0:#B}", 42));
  EXPECT_EQ("-0b101010", fmt::format("{0:#b}", -42));
  EXPECT_EQ("0x42", fmt::format("{0:#x}", 0x42));
  EXPECT_EQ("0X42", fmt::format("{0:#X}", 0x42));
  EXPECT_EQ("-0x42", fmt::format("{0:#x}", -0x42));
  EXPECT_EQ("0", fmt::format("{0:#o}", 0));
  EXPECT_EQ("042", fmt::format("{0:#o}", 042));
  EXPECT_EQ("-042", fmt::format("{0:#o}", -042));
  EXPECT_EQ("42", fmt::format("{0:#}", 42u));
  EXPECT_EQ("0x42", fmt::format("{0:#x}", 0x42u));
  EXPECT_EQ("042", fmt::format("{0:#o}", 042u));

  EXPECT_EQ("-42", fmt::format("{0:#}", -42l));
  EXPECT_EQ("0x42", fmt::format("{0:#x}", 0x42l));
  EXPECT_EQ("-0x42", fmt::format("{0:#x}", -0x42l));
  EXPECT_EQ("042", fmt::format("{0:#o}", 042l));
  EXPECT_EQ("-042", fmt::format("{0:#o}", -042l));
  EXPECT_EQ("42", fmt::format("{0:#}", 42ul));
  EXPECT_EQ("0x42", fmt::format("{0:#x}", 0x42ul));
  EXPECT_EQ("042", fmt::format("{0:#o}", 042ul));

  EXPECT_EQ("-42", fmt::format("{0:#}", -42ll));
  EXPECT_EQ("0x42", fmt::format("{0:#x}", 0x42ll));
  EXPECT_EQ("-0x42", fmt::format("{0:#x}", -0x42ll));
  EXPECT_EQ("042", fmt::format("{0:#o}", 042ll));
  EXPECT_EQ("-042", fmt::format("{0:#o}", -042ll));
  EXPECT_EQ("42", fmt::format("{0:#}", 42ull));
  EXPECT_EQ("0x42", fmt::format("{0:#x}", 0x42ull));
  EXPECT_EQ("042", fmt::format("{0:#o}", 042ull));

  EXPECT_EQ("-42.0", fmt::format("{0:#}", -42.0));
  EXPECT_EQ("-42.0", fmt::format("{0:#}", -42.0l));
  EXPECT_EQ("4.e+01", fmt::format("{:#.0e}", 42.0));
  EXPECT_EQ("0.", fmt::format("{:#.0f}", 0.01));
  EXPECT_EQ("0.50", fmt::format("{:#.2g}", 0.5));
  EXPECT_EQ("0.", fmt::format("{:#.0f}", 0.5));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:#"), 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:#}"), 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:#}"), "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(
      (void)fmt::format(runtime("{0:#}"), reinterpret_cast<void*>(0x42)),
      format_error, "format specifier requires numeric argument");
}

TEST(format_test, zero_flag) {
  EXPECT_EQ("42", fmt::format("{0:0}", 42));
  EXPECT_EQ("-0042", fmt::format("{0:05}", -42));
  EXPECT_EQ("00042", fmt::format("{0:05}", 42u));
  EXPECT_EQ("-0042", fmt::format("{0:05}", -42l));
  EXPECT_EQ("00042", fmt::format("{0:05}", 42ul));
  EXPECT_EQ("-0042", fmt::format("{0:05}", -42ll));
  EXPECT_EQ("00042", fmt::format("{0:05}", 42ull));
  EXPECT_EQ("-000042", fmt::format("{0:07}", -42.0));
  EXPECT_EQ("-000042", fmt::format("{0:07}", -42.0l));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:0"), 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:05}"), 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:05}"), "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(
      (void)fmt::format(runtime("{0:05}"), reinterpret_cast<void*>(0x42)),
      format_error, "format specifier requires numeric argument");
}

TEST(format_test, width) {
  char format_str[buffer_size];
  safe_sprintf(format_str, "{0:%u", UINT_MAX);
  increment(format_str + 3);
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "number is too big");
  size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "number is too big");

  safe_sprintf(format_str, "{0:%u", INT_MAX + 1u);
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "number is too big");
  safe_sprintf(format_str, "{0:%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "number is too big");
  EXPECT_EQ(" -42", fmt::format("{0:4}", -42));
  EXPECT_EQ("   42", fmt::format("{0:5}", 42u));
  EXPECT_EQ("   -42", fmt::format("{0:6}", -42l));
  EXPECT_EQ("     42", fmt::format("{0:7}", 42ul));
  EXPECT_EQ("   -42", fmt::format("{0:6}", -42ll));
  EXPECT_EQ("     42", fmt::format("{0:7}", 42ull));
  EXPECT_EQ("   -1.23", fmt::format("{0:8}", -1.23));
  EXPECT_EQ("    -1.23", fmt::format("{0:9}", -1.23l));
  EXPECT_EQ("    0xcafe",
            fmt::format("{0:10}", reinterpret_cast<void*>(0xcafe)));
  EXPECT_EQ("x          ", fmt::format("{0:11}", 'x'));
  EXPECT_EQ("str         ", fmt::format("{0:12}", "str"));
  EXPECT_EQ(fmt::format("{:*^6}", "🤡"), "**🤡**");
  EXPECT_EQ(fmt::format("{:*^8}", "你好"), "**你好**");
  EXPECT_EQ(fmt::format("{:#6}", 42.0), "  42.0");
  EXPECT_EQ(fmt::format("{:6c}", static_cast<int>('x')), "x     ");
  EXPECT_EQ(fmt::format("{:>06.0f}", 0.00884311), "000000");
}

TEST(format_test, runtime_width) {
  char format_str[buffer_size];
  safe_sprintf(format_str, "{0:{%u", UINT_MAX);
  increment(format_str + 4);
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "invalid format string");
  size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "argument not found");
  format_str[size + 1] = '}';
  format_str[size + 2] = 0;
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "argument not found");

  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{"), 0), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{}"), 0), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{?}}"), 0), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{1}}"), 0), format_error,
                   "argument not found");

  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{0:}}"), 0), format_error,
                   "invalid format string");

  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{1}}"), 0, -1), format_error,
                   "negative width");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{1}}"), 0, (INT_MAX + 1u)),
                   format_error, "number is too big");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{1}}"), 0, -1l), format_error,
                   "negative width");
  if (fmt::detail::const_check(sizeof(long) > sizeof(int))) {
    long value = INT_MAX;
    EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{1}}"), 0, (value + 1)),
                     format_error, "number is too big");
  }
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{1}}"), 0, (INT_MAX + 1ul)),
                   format_error, "number is too big");

  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{1}}"), 0, '0'), format_error,
                   "width is not integer");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{1}}"), 0, 0.0), format_error,
                   "width is not integer");

  EXPECT_EQ(" -42", fmt::format("{0:{1}}", -42, 4));
  EXPECT_EQ("   42", fmt::format("{0:{1}}", 42u, 5));
  EXPECT_EQ("   -42", fmt::format("{0:{1}}", -42l, 6));
  EXPECT_EQ("     42", fmt::format("{0:{1}}", 42ul, 7));
  EXPECT_EQ("   -42", fmt::format("{0:{1}}", -42ll, 6));
  EXPECT_EQ("     42", fmt::format("{0:{1}}", 42ull, 7));
  EXPECT_EQ("   -1.23", fmt::format("{0:{1}}", -1.23, 8));
  EXPECT_EQ("    -1.23", fmt::format("{0:{1}}", -1.23l, 9));
  EXPECT_EQ("    0xcafe",
            fmt::format("{0:{1}}", reinterpret_cast<void*>(0xcafe), 10));
  EXPECT_EQ("x          ", fmt::format("{0:{1}}", 'x', 11));
  EXPECT_EQ("str         ", fmt::format("{0:{1}}", "str", 12));
  EXPECT_EQ(fmt::format("{:{}}", 42, short(4)), "  42");
}

TEST(format_test, precision) {
  char format_str[buffer_size];
  safe_sprintf(format_str, "{0:.%u", UINT_MAX);
  increment(format_str + 4);
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "number is too big");
  size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "number is too big");

  safe_sprintf(format_str, "{0:.%u", INT_MAX + 1u);
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "number is too big");
  safe_sprintf(format_str, "{0:.%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "number is too big");

  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:."), 0), format_error,
                   "missing precision specifier");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.}"), 0), format_error,
                   "missing precision specifier");

  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2"), 0), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2}"), 42), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2f}"), 42), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2}"), 42u), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2f}"), 42u), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2}"), 42l), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2f}"), 42l), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2}"), 42ul), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2f}"), 42ul), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2}"), 42ll), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2f}"), 42ll), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2}"), 42ull), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.2f}"), 42ull), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:3.0}"), 'x'), format_error,
                   "precision not allowed for this argument type");
  EXPECT_EQ("1.2", fmt::format("{0:.2}", 1.2345));
  EXPECT_EQ("1.2", fmt::format("{0:.2}", 1.2345l));
  EXPECT_EQ("1.2e+56", fmt::format("{:.2}", 1.234e56));
  EXPECT_EQ("1.1", fmt::format("{0:.3}", 1.1));
  EXPECT_EQ("1e+00", fmt::format("{:.0e}", 1.0L));
  EXPECT_EQ("  0.0e+00", fmt::format("{:9.1e}", 0.0));
  EXPECT_EQ(
      fmt::format("{:.494}", 4.9406564584124654E-324),
      "4.9406564584124654417656879286822137236505980261432476442558568250067550"
      "727020875186529983636163599237979656469544571773092665671035593979639877"
      "479601078187812630071319031140452784581716784898210368871863605699873072"
      "305000638740915356498438731247339727316961514003171538539807412623856559"
      "117102665855668676818703956031062493194527159149245532930545654440112748"
      "012970999954193198940908041656332452475714786901472678015935523861155013"
      "480352649347201937902681071074917033322268447533357208324319361e-324");
  EXPECT_EQ(
      fmt::format("{:.1074f}", 1.1125369292536e-308),
      "0.0000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000111253692925360019747947051741965785554081512200979"
      "355021686109411883779182127659725163430929750364498219730822952552570601"
      "152163505899912777129583674906301179059298598412303893909188340988729019"
      "014361467448914817838555156840459458527907308695109202499990850735085304"
      "478476991912072201449236975063640913461919914396877093174125167509869762"
      "482369631100360266123742648159508919592746619553246586039571522788247697"
      "156360766271842991667238355464496455107749716934387136380536472531224398"
      "559833794807213172371254492216255558078524900147957309382830827524104234"
      "530961756787819847850302379672357738807808384667004752163416921762619527"
      "462847642037420991432005657440259928195996762610375541867198059294212446"
      "81962777939941034720757232455434770912461317493580281734466552734375");

  std::string outputs[] = {
      "-0X1.41FE3FFE71C9E000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000P+127",
      "-0XA.0FF1FFF38E4F0000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000P+124"};
  EXPECT_THAT(outputs,
              testing::Contains(fmt::format("{:.838A}", -2.14001164E+38)));

  if (std::numeric_limits<long double>::digits == 64) {
    auto ld = (std::numeric_limits<long double>::min)();
    EXPECT_EQ(fmt::format("{:.0}", ld), "3e-4932");
    EXPECT_EQ(
        fmt::format("{:0g}", std::numeric_limits<long double>::denorm_min()),
        "3.6452e-4951");
  }

  EXPECT_EQ("123.", fmt::format("{:#.0f}", 123.0));
  EXPECT_EQ("1.23", fmt::format("{:.02f}", 1.234));
  EXPECT_EQ("0.001", fmt::format("{:.1g}", 0.001));
  EXPECT_EQ("1019666400", fmt::format("{}", 1019666432.0f));
  EXPECT_EQ("1e+01", fmt::format("{:.0e}", 9.5));
  EXPECT_EQ("1.0e-34", fmt::format("{:.1e}", 1e-34));

  EXPECT_THROW_MSG(
      (void)fmt::format(runtime("{0:.2}"), reinterpret_cast<void*>(0xcafe)),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(
      (void)fmt::format(runtime("{0:.2f}"), reinterpret_cast<void*>(0xcafe)),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:.{}e}"), 42.0,
                                     fmt::detail::max_value<int>()),
                   format_error, "number is too big");
  EXPECT_THROW_MSG(
      (void)fmt::format("{:.2147483646f}", -2.2121295195081227E+304),
      format_error, "number is too big");

  EXPECT_EQ("st", fmt::format("{0:.2}", "str"));
  EXPECT_EQ("вожык", fmt::format("{0:.5}", "вожыкі"));
}

TEST(format_test, runtime_precision) {
  char format_str[buffer_size];
  safe_sprintf(format_str, "{0:.{%u", UINT_MAX);
  increment(format_str + 5);
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "invalid format string");
  size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "argument not found");
  format_str[size + 1] = '}';
  format_str[size + 2] = 0;
  EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), 0), format_error,
                   "argument not found");

  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{"), 0), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{}"), 0), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{?}}"), 0), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}"), 0, 0), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 0), format_error,
                   "argument not found");

  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{0:}}"), 0), format_error,
                   "invalid format string");

  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 0, -1), format_error,
                   "negative precision");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 0, (INT_MAX + 1u)),
                   format_error, "number is too big");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 0, -1l), format_error,
                   "negative precision");
  if (fmt::detail::const_check(sizeof(long) > sizeof(int))) {
    long value = INT_MAX;
    EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 0, (value + 1)),
                     format_error, "number is too big");
  }
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 0, (INT_MAX + 1ul)),
                   format_error, "number is too big");

  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 0, '0'), format_error,
                   "precision is not integer");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 0, 0.0), format_error,
                   "precision is not integer");

  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 42, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}f}"), 42, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 42u, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}f}"), 42u, 2),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 42l, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}f}"), 42l, 2),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 42ul, 2),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}f}"), 42ul, 2),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 42ll, 2),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}f}"), 42ll, 2),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"), 42ull, 2),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}f}"), 42ull, 2),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:3.{1}}"), 'x', 0),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_EQ("1.2", fmt::format("{0:.{1}}", 1.2345, 2));
  EXPECT_EQ("1.2", fmt::format("{1:.{0}}", 2, 1.2345l));

  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}}"),
                                     reinterpret_cast<void*>(0xcafe), 2),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:.{1}f}"),
                                     reinterpret_cast<void*>(0xcafe), 2),
                   format_error,
                   "precision not allowed for this argument type");

  EXPECT_EQ("st", fmt::format("{0:.{1}}", "str", 2));
}

TEST(format_test, format_bool) {
  EXPECT_EQ("true", fmt::format("{}", true));
  EXPECT_EQ("false", fmt::format("{}", false));
  EXPECT_EQ("1", fmt::format("{:d}", true));
  EXPECT_EQ("true ", fmt::format("{:5}", true));
  EXPECT_EQ("true", fmt::format("{:s}", true));
  EXPECT_EQ("false", fmt::format("{:s}", false));
  EXPECT_EQ("false ", fmt::format("{:6s}", false));
}

TEST(format_test, format_short) {
  short s = 42;
  EXPECT_EQ("42", fmt::format("{0:d}", s));
  unsigned short us = 42;
  EXPECT_EQ("42", fmt::format("{0:d}", us));
}

template <typename T>
void check_unknown_types(const T& value, const char* types, const char*) {
  char format_str[buffer_size];
  const char* special = ".0123456789L?}";
  for (int i = CHAR_MIN; i <= CHAR_MAX; ++i) {
    char c = static_cast<char>(i);
    if (std::strchr(types, c) || std::strchr(special, c) || !c) continue;
    safe_sprintf(format_str, "{0:10%c}", c);
    const char* message = "invalid type specifier";
    EXPECT_THROW_MSG((void)fmt::format(runtime(format_str), value),
                     format_error, message)
        << format_str << " " << message;
  }
}

TEST(format_test, format_int) {
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:v"), 42), format_error,
                   "invalid type specifier");
  check_unknown_types(42, "bBdoxXnLc", "integer");
  EXPECT_EQ("x", fmt::format("{:c}", static_cast<int>('x')));
}

TEST(format_test, format_bin) {
  EXPECT_EQ("0", fmt::format("{0:b}", 0));
  EXPECT_EQ("101010", fmt::format("{0:b}", 42));
  EXPECT_EQ("101010", fmt::format("{0:b}", 42u));
  EXPECT_EQ("-101010", fmt::format("{0:b}", -42));
  EXPECT_EQ("11000000111001", fmt::format("{0:b}", 12345));
  EXPECT_EQ("10010001101000101011001111000", fmt::format("{0:b}", 0x12345678));
  EXPECT_EQ("10010000101010111100110111101111",
            fmt::format("{0:b}", 0x90ABCDEF));
  EXPECT_EQ("11111111111111111111111111111111",
            fmt::format("{0:b}", max_value<uint32_t>()));
}

#if FMT_USE_INT128
constexpr auto int128_max = static_cast<__int128_t>(
    (static_cast<__uint128_t>(1) << ((__SIZEOF_INT128__ * CHAR_BIT) - 1)) - 1);
constexpr auto int128_min = -int128_max - 1;

constexpr auto uint128_max = ~static_cast<__uint128_t>(0);
#endif

TEST(format_test, format_dec) {
  EXPECT_EQ("0", fmt::format("{0}", 0));
  EXPECT_EQ("42", fmt::format("{0}", 42));
  EXPECT_EQ("42", fmt::format("{0:d}", 42));
  EXPECT_EQ("42", fmt::format("{0}", 42u));
  EXPECT_EQ("-42", fmt::format("{0}", -42));
  EXPECT_EQ("12345", fmt::format("{0}", 12345));
  EXPECT_EQ("67890", fmt::format("{0}", 67890));
#if FMT_USE_INT128
  EXPECT_EQ("0", fmt::format("{0}", static_cast<__int128_t>(0)));
  EXPECT_EQ("0", fmt::format("{0}", static_cast<__uint128_t>(0)));
  EXPECT_EQ("9223372036854775808",
            fmt::format("{0}", static_cast<__int128_t>(INT64_MAX) + 1));
  EXPECT_EQ("-9223372036854775809",
            fmt::format("{0}", static_cast<__int128_t>(INT64_MIN) - 1));
  EXPECT_EQ("18446744073709551616",
            fmt::format("{0}", static_cast<__int128_t>(UINT64_MAX) + 1));
  EXPECT_EQ("170141183460469231731687303715884105727",
            fmt::format("{0}", int128_max));
  EXPECT_EQ("-170141183460469231731687303715884105728",
            fmt::format("{0}", int128_min));
  EXPECT_EQ("340282366920938463463374607431768211455",
            fmt::format("{0}", uint128_max));
#endif

  char buffer[buffer_size];
  safe_sprintf(buffer, "%d", INT_MIN);
  EXPECT_EQ(buffer, fmt::format("{0}", INT_MIN));
  safe_sprintf(buffer, "%d", INT_MAX);
  EXPECT_EQ(buffer, fmt::format("{0}", INT_MAX));
  safe_sprintf(buffer, "%u", UINT_MAX);
  EXPECT_EQ(buffer, fmt::format("{0}", UINT_MAX));
  safe_sprintf(buffer, "%ld", 0 - static_cast<unsigned long>(LONG_MIN));
  EXPECT_EQ(buffer, fmt::format("{0}", LONG_MIN));
  safe_sprintf(buffer, "%ld", LONG_MAX);
  EXPECT_EQ(buffer, fmt::format("{0}", LONG_MAX));
  safe_sprintf(buffer, "%lu", ULONG_MAX);
  EXPECT_EQ(buffer, fmt::format("{0}", ULONG_MAX));
}

TEST(format_test, format_hex) {
  EXPECT_EQ("0", fmt::format("{0:x}", 0));
  EXPECT_EQ("42", fmt::format("{0:x}", 0x42));
  EXPECT_EQ("42", fmt::format("{0:x}", 0x42u));
  EXPECT_EQ("-42", fmt::format("{0:x}", -0x42));
  EXPECT_EQ("12345678", fmt::format("{0:x}", 0x12345678));
  EXPECT_EQ("90abcdef", fmt::format("{0:x}", 0x90abcdef));
  EXPECT_EQ("12345678", fmt::format("{0:X}", 0x12345678));
  EXPECT_EQ("90ABCDEF", fmt::format("{0:X}", 0x90ABCDEF));
#if FMT_USE_INT128
  EXPECT_EQ("0", fmt::format("{0:x}", static_cast<__int128_t>(0)));
  EXPECT_EQ("0", fmt::format("{0:x}", static_cast<__uint128_t>(0)));
  EXPECT_EQ("8000000000000000",
            fmt::format("{0:x}", static_cast<__int128_t>(INT64_MAX) + 1));
  EXPECT_EQ("-8000000000000001",
            fmt::format("{0:x}", static_cast<__int128_t>(INT64_MIN) - 1));
  EXPECT_EQ("10000000000000000",
            fmt::format("{0:x}", static_cast<__int128_t>(UINT64_MAX) + 1));
  EXPECT_EQ("7fffffffffffffffffffffffffffffff",
            fmt::format("{0:x}", int128_max));
  EXPECT_EQ("-80000000000000000000000000000000",
            fmt::format("{0:x}", int128_min));
  EXPECT_EQ("ffffffffffffffffffffffffffffffff",
            fmt::format("{0:x}", uint128_max));
#endif

  char buffer[buffer_size];
  safe_sprintf(buffer, "-%x", 0 - static_cast<unsigned>(INT_MIN));
  EXPECT_EQ(buffer, fmt::format("{0:x}", INT_MIN));
  safe_sprintf(buffer, "%x", INT_MAX);
  EXPECT_EQ(buffer, fmt::format("{0:x}", INT_MAX));
  safe_sprintf(buffer, "%x", UINT_MAX);
  EXPECT_EQ(buffer, fmt::format("{0:x}", UINT_MAX));
  safe_sprintf(buffer, "-%lx", 0 - static_cast<unsigned long>(LONG_MIN));
  EXPECT_EQ(buffer, fmt::format("{0:x}", LONG_MIN));
  safe_sprintf(buffer, "%lx", LONG_MAX);
  EXPECT_EQ(buffer, fmt::format("{0:x}", LONG_MAX));
  safe_sprintf(buffer, "%lx", ULONG_MAX);
  EXPECT_EQ(buffer, fmt::format("{0:x}", ULONG_MAX));
}

TEST(format_test, format_oct) {
  EXPECT_EQ("0", fmt::format("{0:o}", 0));
  EXPECT_EQ("42", fmt::format("{0:o}", 042));
  EXPECT_EQ("42", fmt::format("{0:o}", 042u));
  EXPECT_EQ("-42", fmt::format("{0:o}", -042));
  EXPECT_EQ("12345670", fmt::format("{0:o}", 012345670));
#if FMT_USE_INT128
  EXPECT_EQ("0", fmt::format("{0:o}", static_cast<__int128_t>(0)));
  EXPECT_EQ("0", fmt::format("{0:o}", static_cast<__uint128_t>(0)));
  EXPECT_EQ("1000000000000000000000",
            fmt::format("{0:o}", static_cast<__int128_t>(INT64_MAX) + 1));
  EXPECT_EQ("-1000000000000000000001",
            fmt::format("{0:o}", static_cast<__int128_t>(INT64_MIN) - 1));
  EXPECT_EQ("2000000000000000000000",
            fmt::format("{0:o}", static_cast<__int128_t>(UINT64_MAX) + 1));
  EXPECT_EQ("1777777777777777777777777777777777777777777",
            fmt::format("{0:o}", int128_max));
  EXPECT_EQ("-2000000000000000000000000000000000000000000",
            fmt::format("{0:o}", int128_min));
  EXPECT_EQ("3777777777777777777777777777777777777777777",
            fmt::format("{0:o}", uint128_max));
#endif

  char buffer[buffer_size];
  safe_sprintf(buffer, "-%o", 0 - static_cast<unsigned>(INT_MIN));
  EXPECT_EQ(buffer, fmt::format("{0:o}", INT_MIN));
  safe_sprintf(buffer, "%o", INT_MAX);
  EXPECT_EQ(buffer, fmt::format("{0:o}", INT_MAX));
  safe_sprintf(buffer, "%o", UINT_MAX);
  EXPECT_EQ(buffer, fmt::format("{0:o}", UINT_MAX));
  safe_sprintf(buffer, "-%lo", 0 - static_cast<unsigned long>(LONG_MIN));
  EXPECT_EQ(buffer, fmt::format("{0:o}", LONG_MIN));
  safe_sprintf(buffer, "%lo", LONG_MAX);
  EXPECT_EQ(buffer, fmt::format("{0:o}", LONG_MAX));
  safe_sprintf(buffer, "%lo", ULONG_MAX);
  EXPECT_EQ(buffer, fmt::format("{0:o}", ULONG_MAX));
}

TEST(format_test, format_int_locale) {
  EXPECT_EQ("1234", fmt::format("{:L}", 1234));
}

TEST(format_test, format_float) {
  EXPECT_EQ("0", fmt::format("{}", 0.0f));
  EXPECT_EQ("392.500000", fmt::format("{0:f}", 392.5f));
}

TEST(format_test, format_double) {
  EXPECT_EQ(fmt::format("{}", 0.0), "0");
  check_unknown_types(1.2, "eEfFgGaAnL%", "double");
  EXPECT_EQ(fmt::format("{:}", 0.0), "0");
  EXPECT_EQ(fmt::format("{:f}", 0.0), "0.000000");
  EXPECT_EQ(fmt::format("{:g}", 0.0), "0");
  EXPECT_EQ(fmt::format("{:}", 392.65), "392.65");
  EXPECT_EQ(fmt::format("{:g}", 392.65), "392.65");
  EXPECT_EQ(fmt::format("{:G}", 392.65), "392.65");
  EXPECT_EQ(fmt::format("{:g}", 4.9014e6), "4.9014e+06");
  EXPECT_EQ(fmt::format("{:f}", 392.65), "392.650000");
  EXPECT_EQ(fmt::format("{:F}", 392.65), "392.650000");
  EXPECT_EQ(fmt::format("{:L}", 42.0), "42");
  EXPECT_EQ(fmt::format("{:24a}", 4.2), "    0x1.0cccccccccccdp+2");
  EXPECT_EQ(fmt::format("{:<24a}", 4.2), "0x1.0cccccccccccdp+2    ");
  EXPECT_EQ(fmt::format("{0:e}", 392.65), "3.926500e+02");
  EXPECT_EQ(fmt::format("{0:E}", 392.65), "3.926500E+02");
  EXPECT_EQ(fmt::format("{0:+010.4g}", 392.65), "+0000392.6");
  char buffer[buffer_size];
  safe_sprintf(buffer, "%a", -42.0);
  EXPECT_EQ(fmt::format("{:a}", -42.0), buffer);
  safe_sprintf(buffer, "%A", -42.0);
  EXPECT_EQ(fmt::format("{:A}", -42.0), buffer);
  EXPECT_EQ(fmt::format("{:f}", 9223372036854775807.0),
            "9223372036854775808.000000");
}

TEST(format_test, precision_rounding) {
  EXPECT_EQ("0", fmt::format("{:.0f}", 0.0));
  EXPECT_EQ("0", fmt::format("{:.0f}", 0.01));
  EXPECT_EQ("0", fmt::format("{:.0f}", 0.1));
  EXPECT_EQ("0.000", fmt::format("{:.3f}", 0.00049));
  EXPECT_EQ("0.001", fmt::format("{:.3f}", 0.0005));
  EXPECT_EQ("0.001", fmt::format("{:.3f}", 0.00149));
  EXPECT_EQ("0.002", fmt::format("{:.3f}", 0.0015));
  EXPECT_EQ("1.000", fmt::format("{:.3f}", 0.9999));
  EXPECT_EQ("0.00123", fmt::format("{:.3}", 0.00123));
  EXPECT_EQ("0.1", fmt::format("{:.16g}", 0.1));
  EXPECT_EQ("1", fmt::format("{:.0}", 1.0));
  EXPECT_EQ("225.51575035152063720",
            fmt::format("{:.17f}", 225.51575035152064));
  EXPECT_EQ("-761519619559038.2", fmt::format("{:.1f}", -761519619559038.2));
  EXPECT_EQ("1.9156918820264798e-56",
            fmt::format("{}", 1.9156918820264798e-56));
  EXPECT_EQ("0.0000", fmt::format("{:.4f}", 7.2809479766055470e-15));

  // Trigger a rounding error in Grisu by a specially chosen number.
  EXPECT_EQ("3788512123356.985352", fmt::format("{:f}", 3788512123356.985352));
}

TEST(format_test, prettify_float) {
  EXPECT_EQ("0.0001", fmt::format("{}", 1e-4));
  EXPECT_EQ("1e-05", fmt::format("{}", 1e-5));
  EXPECT_EQ("1000000000000000", fmt::format("{}", 1e15));
  EXPECT_EQ("1e+16", fmt::format("{}", 1e16));
  EXPECT_EQ("9.999e-05", fmt::format("{}", 9.999e-5));
  EXPECT_EQ("10000000000", fmt::format("{}", 1e10));
  EXPECT_EQ("100000000000", fmt::format("{}", 1e11));
  EXPECT_EQ("12340000000", fmt::format("{}", 1234e7));
  EXPECT_EQ("12.34", fmt::format("{}", 1234e-2));
  EXPECT_EQ("0.001234", fmt::format("{}", 1234e-6));
  EXPECT_EQ("0.1", fmt::format("{}", 0.1f));
  EXPECT_EQ("0.10000000149011612", fmt::format("{}", double(0.1f)));
  EXPECT_EQ("1.3563156e-19", fmt::format("{}", 1.35631564e-19f));
}

TEST(format_test, format_nan) {
  double nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ("nan", fmt::format("{}", nan));
  EXPECT_EQ("+nan", fmt::format("{:+}", nan));
  EXPECT_EQ("  +nan", fmt::format("{:+06}", nan));
  EXPECT_EQ("+nan  ", fmt::format("{:<+06}", nan));
  EXPECT_EQ(" +nan ", fmt::format("{:^+06}", nan));
  EXPECT_EQ("  +nan", fmt::format("{:>+06}", nan));
  if (std::signbit(-nan)) {
    EXPECT_EQ("-nan", fmt::format("{}", -nan));
    EXPECT_EQ("  -nan", fmt::format("{:+06}", -nan));
  } else {
    fmt::print("Warning: compiler doesn't handle negative NaN correctly");
  }
  EXPECT_EQ(" nan", fmt::format("{: }", nan));
  EXPECT_EQ("NAN", fmt::format("{:F}", nan));
  EXPECT_EQ("nan    ", fmt::format("{:<7}", nan));
  EXPECT_EQ("  nan  ", fmt::format("{:^7}", nan));
  EXPECT_EQ("    nan", fmt::format("{:>7}", nan));
}

TEST(format_test, format_infinity) {
  double inf = std::numeric_limits<double>::infinity();
  EXPECT_EQ("inf", fmt::format("{}", inf));
  EXPECT_EQ("+inf", fmt::format("{:+}", inf));
  EXPECT_EQ("-inf", fmt::format("{}", -inf));
  EXPECT_EQ("  +inf", fmt::format("{:+06}", inf));
  EXPECT_EQ("  -inf", fmt::format("{:+06}", -inf));
  EXPECT_EQ("+inf  ", fmt::format("{:<+06}", inf));
  EXPECT_EQ(" +inf ", fmt::format("{:^+06}", inf));
  EXPECT_EQ("  +inf", fmt::format("{:>+06}", inf));
  EXPECT_EQ(" inf", fmt::format("{: }", inf));
  EXPECT_EQ("INF", fmt::format("{:F}", inf));
  EXPECT_EQ("inf    ", fmt::format("{:<7}", inf));
  EXPECT_EQ("  inf  ", fmt::format("{:^7}", inf));
  EXPECT_EQ("    inf", fmt::format("{:>7}", inf));
}

TEST(format_test, format_long_double) {
  EXPECT_EQ("0", fmt::format("{0:}", 0.0l));
  EXPECT_EQ("0.000000", fmt::format("{0:f}", 0.0l));
  EXPECT_EQ("392.65", fmt::format("{0:}", 392.65l));
  EXPECT_EQ("392.65", fmt::format("{0:g}", 392.65l));
  EXPECT_EQ("392.65", fmt::format("{0:G}", 392.65l));
  EXPECT_EQ("392.650000", fmt::format("{0:f}", 392.65l));
  EXPECT_EQ("392.650000", fmt::format("{0:F}", 392.65l));
  char buffer[buffer_size];
  safe_sprintf(buffer, "%Le", 392.65l);
  EXPECT_EQ(buffer, fmt::format("{0:e}", 392.65l));
  EXPECT_EQ("+0000392.6", fmt::format("{0:+010.4g}", 392.64l));
  safe_sprintf(buffer, "%La", 3.31l);
  EXPECT_EQ(buffer, fmt::format("{:a}", 3.31l));
}

TEST(format_test, format_char) {
  const char types[] = "cbBdoxX";
  check_unknown_types('a', types, "char");
  EXPECT_EQ("a", fmt::format("{0}", 'a'));
  EXPECT_EQ("z", fmt::format("{0:c}", 'z'));
  int n = 'x';
  for (const char* type = types + 1; *type; ++type) {
    std::string format_str = fmt::format("{{:{}}}", *type);
    EXPECT_EQ(fmt::format(runtime(format_str), n),
              fmt::format(runtime(format_str), 'x'))
        << format_str;
  }
  EXPECT_EQ(fmt::format("{:02X}", n), fmt::format("{:02X}", 'x'));

  EXPECT_EQ("\n", fmt::format("{}", '\n'));
  EXPECT_EQ("'\\n'", fmt::format("{:?}", '\n'));
}

TEST(format_test, format_volatile_char) {
  volatile char c = 'x';
  EXPECT_EQ("x", fmt::format("{}", c));
}

TEST(format_test, format_unsigned_char) {
  EXPECT_EQ("42", fmt::format("{}", static_cast<unsigned char>(42)));
  EXPECT_EQ("42", fmt::format("{}", static_cast<uint8_t>(42)));
}

TEST(format_test, format_cstring) {
  check_unknown_types("test", "sp", "string");
  EXPECT_EQ("test", fmt::format("{0}", "test"));
  EXPECT_EQ("test", fmt::format("{0:s}", "test"));
  char nonconst[] = "nonconst";
  EXPECT_EQ("nonconst", fmt::format("{0}", nonconst));
  EXPECT_THROW_MSG(
      (void)fmt::format(runtime("{0}"), static_cast<const char*>(nullptr)),
      format_error, "string pointer is null");
}

void function_pointer_test(int, double, std::string) {}

TEST(format_test, format_pointer) {
  check_unknown_types(reinterpret_cast<void*>(0x1234), "p", "pointer");
  EXPECT_EQ("0x0", fmt::format("{0}", static_cast<void*>(nullptr)));
  EXPECT_EQ("0x1234", fmt::format("{0}", reinterpret_cast<void*>(0x1234)));
  EXPECT_EQ("0x1234", fmt::format("{0:p}", reinterpret_cast<void*>(0x1234)));
  // On CHERI (or other fat-pointer) systems, the size of a pointer is greater
  // than the size an integer that can hold a virtual address.  There is no
  // portable address-as-an-integer type (yet) in C++, so we use `size_t` as
  // the closest equivalent for now.
  EXPECT_EQ("0x" + std::string(sizeof(size_t) * CHAR_BIT / 4, 'f'),
            fmt::format("{0}", reinterpret_cast<void*>(~uintptr_t())));
  EXPECT_EQ("0x1234",
            fmt::format("{}", fmt::ptr(reinterpret_cast<int*>(0x1234))));
  std::unique_ptr<int> up(new int(1));
  EXPECT_EQ(fmt::format("{}", fmt::ptr(up.get())),
            fmt::format("{}", fmt::ptr(up)));
  std::shared_ptr<int> sp(new int(1));
  EXPECT_EQ(fmt::format("{}", fmt::ptr(sp.get())),
            fmt::format("{}", fmt::ptr(sp)));
  EXPECT_EQ(fmt::format("{}", fmt::detail::bit_cast<const void*>(
                                  &function_pointer_test)),
            fmt::format("{}", fmt::ptr(function_pointer_test)));
  EXPECT_EQ("0x0", fmt::format("{}", nullptr));
}

TEST(format_test, write_uintptr_fallback) {
  // Test that formatting a pointer by converting it to uint128_fallback works.
  // This is needed to support systems without uintptr_t.
  auto s = std::string();
  fmt::detail::write_ptr<char>(
      std::back_inserter(s),
      fmt::detail::bit_cast<fmt::detail::uint128_fallback>(
          reinterpret_cast<void*>(0xface)),
      nullptr);
  EXPECT_EQ(s, "0xface");
}

enum class color { red, green, blue };

namespace test_ns {
enum class color { red, green, blue };
using fmt::enums::format_as;
}  // namespace test_ns

TEST(format_test, format_enum_class) {
  EXPECT_EQ(fmt::format("{}", fmt::underlying(color::red)), "0");
  EXPECT_EQ(fmt::format("{}", test_ns::color::red), "0");
}

TEST(format_test, format_string) {
  EXPECT_EQ(fmt::format("{0}", std::string("test")), "test");
  EXPECT_EQ(fmt::format("{0}", std::string("test")), "test");
  EXPECT_EQ(fmt::format("{:?}", std::string("test")), "\"test\"");
  EXPECT_EQ(fmt::format("{:*^10?}", std::string("test")), "**\"test\"**");
  EXPECT_EQ(fmt::format("{:?}", std::string("\test")), "\"\\test\"");
  EXPECT_THROW((void)fmt::format(fmt::runtime("{:x}"), std::string("test")),
               fmt::format_error);
}

TEST(format_test, format_string_view) {
  EXPECT_EQ("test", fmt::format("{}", string_view("test")));
  EXPECT_EQ("\"t\\nst\"", fmt::format("{:?}", string_view("t\nst")));
  EXPECT_EQ("", fmt::format("{}", string_view()));
}

#ifdef FMT_USE_STRING_VIEW
struct string_viewable {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<string_viewable> : formatter<std::string_view> {
  auto format(string_viewable, format_context& ctx) -> decltype(ctx.out()) {
    return formatter<std::string_view>::format("foo", ctx);
  }
};
FMT_END_NAMESPACE

TEST(format_test, format_std_string_view) {
  EXPECT_EQ("test", fmt::format("{}", std::string_view("test")));
  EXPECT_EQ("foo", fmt::format("{}", string_viewable()));
}

struct explicitly_convertible_to_std_string_view {
  explicit operator std::string_view() const { return "foo"; }
};

template <>
struct fmt::formatter<explicitly_convertible_to_std_string_view>
    : formatter<std::string_view> {
  auto format(explicitly_convertible_to_std_string_view v, format_context& ctx)
      -> decltype(ctx.out()) {
    return format_to(ctx.out(), "'{}'", std::string_view(v));
  }
};

TEST(format_test, format_explicitly_convertible_to_std_string_view) {
  EXPECT_EQ("'foo'",
            fmt::format("{}", explicitly_convertible_to_std_string_view()));
}
#endif

struct converible_to_anything {
  template <typename T> operator T() const { return T(); }
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<converible_to_anything> {
  FMT_CONSTEXPR auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  auto format(converible_to_anything, format_context& ctx)
      -> decltype(ctx.out()) {
    return format_to(ctx.out(), "foo");
  }
};
FMT_END_NAMESPACE

TEST(format_test, format_convertible_to_anything) {
  EXPECT_EQ("foo", fmt::format("{}", converible_to_anything()));
}

class Answer {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<date> {
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == 'd') ++it;
    return it;
  }

  auto format(const date& d, format_context& ctx) -> decltype(ctx.out()) {
    format_to(ctx.out(), "{}-{}-{}", d.year(), d.month(), d.day());
    return ctx.out();
  }
};

template <> struct formatter<Answer> : formatter<int> {
  template <typename FormatContext>
  auto format(Answer, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<int>::format(42, ctx);
  }
};
FMT_END_NAMESPACE

TEST(format_test, format_custom) {
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:s}"), date(2012, 12, 9)),
                   format_error, "unknown format specifier");
  EXPECT_EQ("42", fmt::format("{0}", Answer()));
  EXPECT_EQ("0042", fmt::format("{:04}", Answer()));
}

TEST(format_test, format_to_custom) {
  char buf[10] = {};
  auto end =
      &*fmt::format_to(fmt::detail::make_checked(buf, 10), "{}", Answer());
  EXPECT_EQ(end, buf + 2);
  EXPECT_STREQ(buf, "42");
}

TEST(format_test, format_string_from_speed_test) {
  EXPECT_EQ("1.2340000000:0042:+3.13:str:0x3e8:X:%",
            fmt::format("{0:0.10f}:{1:04}:{2:+g}:{3}:{4}:{5}:%", 1.234, 42,
                        3.13, "str", reinterpret_cast<void*>(1000), 'X'));
}

TEST(format_test, format_examples) {
  std::string message = fmt::format("The answer is {}", 42);
  EXPECT_EQ("The answer is 42", message);

  EXPECT_EQ("42", fmt::format("{}", 42));

  memory_buffer out;
  format_to(std::back_inserter(out), "The answer is {}.", 42);
  EXPECT_EQ("The answer is 42.", to_string(out));

  const char* filename = "nonexistent";
  FILE* ftest = safe_fopen(filename, "r");
  if (ftest) fclose(ftest);
  int error_code = errno;
  EXPECT_TRUE(ftest == nullptr);
  EXPECT_SYSTEM_ERROR(
      {
        FILE* f = safe_fopen(filename, "r");
        if (!f)
          throw fmt::system_error(errno, "Cannot open file '{}'", filename);
        fclose(f);
      },
      error_code, "Cannot open file 'nonexistent'");

  EXPECT_EQ("First, thou shalt count to three",
            fmt::format("First, thou shalt count to {0}", "three"));
  EXPECT_EQ("Bring me a shrubbery", fmt::format("Bring me a {}", "shrubbery"));
  EXPECT_EQ("From 1 to 3", fmt::format("From {} to {}", 1, 3));

  char buffer[buffer_size];
  safe_sprintf(buffer, "%03.2f", -1.2);
  EXPECT_EQ(buffer, fmt::format("{:03.2f}", -1.2));

  EXPECT_EQ("a, b, c", fmt::format("{0}, {1}, {2}", 'a', 'b', 'c'));
  EXPECT_EQ("a, b, c", fmt::format("{}, {}, {}", 'a', 'b', 'c'));
  EXPECT_EQ("c, b, a", fmt::format("{2}, {1}, {0}", 'a', 'b', 'c'));
  EXPECT_EQ("abracadabra", fmt::format("{0}{1}{0}", "abra", "cad"));

  EXPECT_EQ("left aligned                  ",
            fmt::format("{:<30}", "left aligned"));
  EXPECT_EQ("                 right aligned",
            fmt::format("{:>30}", "right aligned"));
  EXPECT_EQ("           centered           ",
            fmt::format("{:^30}", "centered"));
  EXPECT_EQ("***********centered***********",
            fmt::format("{:*^30}", "centered"));

  EXPECT_EQ("+3.140000; -3.140000", fmt::format("{:+f}; {:+f}", 3.14, -3.14));
  EXPECT_EQ(" 3.140000; -3.140000", fmt::format("{: f}; {: f}", 3.14, -3.14));
  EXPECT_EQ("3.140000; -3.140000", fmt::format("{:-f}; {:-f}", 3.14, -3.14));

  EXPECT_EQ("int: 42;  hex: 2a;  oct: 52",
            fmt::format("int: {0:d};  hex: {0:x};  oct: {0:o}", 42));
  EXPECT_EQ("int: 42;  hex: 0x2a;  oct: 052",
            fmt::format("int: {0:d};  hex: {0:#x};  oct: {0:#o}", 42));

  EXPECT_EQ("The answer is 42", fmt::format("The answer is {}", 42));
  EXPECT_THROW_MSG(
      (void)fmt::format(runtime("The answer is {:d}"), "forty-two"),
      format_error, "invalid type specifier");

  EXPECT_WRITE(
      stdout, fmt::print("{}", std::numeric_limits<double>::infinity()), "inf");
}

TEST(format_test, print) {
  EXPECT_WRITE(stdout, fmt::print("Don't {}!", "panic"), "Don't panic!");
  EXPECT_WRITE(stderr, fmt::print(stderr, "Don't {}!", "panic"),
               "Don't panic!");
}

TEST(format_test, variadic) {
  EXPECT_EQ("abc1", fmt::format("{}c{}", "ab", 1));
}

TEST(format_test, dynamic) {
  using ctx = fmt::format_context;
  auto args = std::vector<fmt::basic_format_arg<ctx>>();
  args.emplace_back(fmt::detail::make_arg<ctx>(42));
  args.emplace_back(fmt::detail::make_arg<ctx>("abc1"));
  args.emplace_back(fmt::detail::make_arg<ctx>(1.5f));

  std::string result = fmt::vformat(
      "{} and {} and {}",
      fmt::format_args(args.data(), static_cast<int>(args.size())));

  EXPECT_EQ("42 and abc1 and 1.5", result);
}

TEST(format_test, bytes) {
  auto s = fmt::format("{:10}", fmt::bytes("ёжик"));
  EXPECT_EQ("ёжик  ", s);
  EXPECT_EQ(10, s.size());
}

TEST(format_test, group_digits_view) {
  EXPECT_EQ(fmt::format("{}", fmt::group_digits(10000000)), "10,000,000");
  EXPECT_EQ(fmt::format("{:8}", fmt::group_digits(1000)), "   1,000");
}

enum test_enum { foo, bar };
auto format_as(test_enum e) -> int { return e; }

TEST(format_test, join) {
  using fmt::join;
  int v1[3] = {1, 2, 3};
  auto v2 = std::vector<float>();
  v2.push_back(1.2f);
  v2.push_back(3.4f);
  void* v3[2] = {&v1[0], &v1[1]};

  EXPECT_EQ("(1, 2, 3)", fmt::format("({})", join(v1, v1 + 3, ", ")));
  EXPECT_EQ("(1)", fmt::format("({})", join(v1, v1 + 1, ", ")));
  EXPECT_EQ("()", fmt::format("({})", join(v1, v1, ", ")));
  EXPECT_EQ("(001, 002, 003)", fmt::format("({:03})", join(v1, v1 + 3, ", ")));
  EXPECT_EQ("(+01.20, +03.40)",
            fmt::format("({:+06.2f})", join(v2.begin(), v2.end(), ", ")));

  EXPECT_EQ("1, 2, 3", fmt::format("{0:{1}}", join(v1, v1 + 3, ", "), 1));

  EXPECT_EQ(fmt::format("{}, {}", v3[0], v3[1]),
            fmt::format("{}", join(v3, v3 + 2, ", ")));

  EXPECT_EQ("(1, 2, 3)", fmt::format("({})", join(v1, ", ")));
  EXPECT_EQ("(+01.20, +03.40)", fmt::format("({:+06.2f})", join(v2, ", ")));

  auto v4 = std::vector<test_enum>{foo, bar, foo};
  EXPECT_EQ("0 1 0", fmt::format("{}", join(v4, " ")));
}

#ifdef __cpp_lib_byte
TEST(format_test, format_byte) {
  using arg_mapper = fmt::detail::arg_mapper<fmt::format_context>;
  EXPECT_EQ(arg_mapper().map(std::byte(42)), 42);
  EXPECT_EQ(fmt::format("{}", std::byte(42)), "42");
}

TEST(format_test, join_bytes) {
  auto v = std::vector<std::byte>{std::byte(1), std::byte(2), std::byte(3)};
  EXPECT_EQ(fmt::format("{}", fmt::join(v, ", ")), "1, 2, 3");
}
#endif

std::string vformat_message(int id, const char* format, fmt::format_args args) {
  auto buffer = fmt::memory_buffer();
  format_to(fmt::appender(buffer), "[{}] ", id);
  vformat_to(fmt::appender(buffer), format, args);
  return to_string(buffer);
}

template <typename... Args>
std::string format_message(int id, const char* format, const Args&... args) {
  auto va = fmt::make_format_args(args...);
  return vformat_message(id, format, va);
}

TEST(format_test, format_message_example) {
  EXPECT_EQ("[42] something happened",
            format_message(42, "{} happened", "something"));
}

template <typename... Args>
void print_error(const char* file, int line, const char* format,
                 const Args&... args) {
  fmt::print("{}: {}: ", file, line);
  fmt::print(format, args...);
}

TEST(format_test, unpacked_args) {
  EXPECT_EQ("0123456789abcdefg",
            fmt::format("{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}", 0, 1, 2, 3, 4, 5,
                        6, 7, 8, 9, 'a', 'b', 'c', 'd', 'e', 'f', 'g'));
}

struct string_like {};
fmt::string_view to_string_view(string_like) { return "foo"; }

constexpr char with_null[3] = {'{', '}', '\0'};
constexpr char no_null[2] = {'{', '}'};
static constexpr const char static_with_null[3] = {'{', '}', '\0'};
static constexpr const char static_no_null[2] = {'{', '}'};

TEST(format_test, compile_time_string) {
  EXPECT_EQ("foo", fmt::format(FMT_STRING("foo")));
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), 42));
  EXPECT_EQ("foo", fmt::format(FMT_STRING("{}"), string_like()));

#if FMT_USE_NONTYPE_TEMPLATE_ARGS
  using namespace fmt::literals;
  EXPECT_EQ("foobar", fmt::format(FMT_STRING("{foo}{bar}"), "bar"_a = "bar",
                                  "foo"_a = "foo"));
  EXPECT_EQ("", fmt::format(FMT_STRING("")));
  EXPECT_EQ("", fmt::format(FMT_STRING(""), "arg"_a = 42));
  EXPECT_EQ("42", fmt::format(FMT_STRING("{answer}"), "answer"_a = Answer()));
#endif

  (void)static_with_null;
  (void)static_no_null;
#ifndef _MSC_VER
  EXPECT_EQ("42", fmt::format(FMT_STRING(static_with_null), 42));
  EXPECT_EQ("42", fmt::format(FMT_STRING(static_no_null), 42));
#endif

  (void)with_null;
  (void)no_null;
#if FMT_CPLUSPLUS >= 201703L
  EXPECT_EQ("42", fmt::format(FMT_STRING(with_null), 42));
  EXPECT_EQ("42", fmt::format(FMT_STRING(no_null), 42));
#endif
#if defined(FMT_USE_STRING_VIEW) && FMT_CPLUSPLUS >= 201703L
  EXPECT_EQ("42", fmt::format(FMT_STRING(std::string_view("{}")), 42));
#endif
}

TEST(format_test, custom_format_compile_time_string) {
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), Answer()));
  auto answer = Answer();
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), answer));
  char buf[10] = {};
  fmt::format_to(buf, FMT_STRING("{}"), answer);
  const Answer const_answer = Answer();
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), const_answer));
}

#if FMT_USE_USER_DEFINED_LITERALS
TEST(format_test, named_arg_udl) {
  using namespace fmt::literals;
  auto udl_a = fmt::format("{first}{second}{first}{third}", "first"_a = "abra",
                           "second"_a = "cad", "third"_a = 99);
  EXPECT_EQ(
      fmt::format("{first}{second}{first}{third}", fmt::arg("first", "abra"),
                  fmt::arg("second", "cad"), fmt::arg("third", 99)),
      udl_a);

  EXPECT_EQ("42", fmt::format("{answer}", "answer"_a = Answer()));
}
#endif  // FMT_USE_USER_DEFINED_LITERALS

TEST(format_test, enum) { EXPECT_EQ("0", fmt::format("{}", foo)); }

TEST(format_test, formatter_not_specialized) {
  static_assert(!fmt::has_formatter<fmt::formatter<test_enum>,
                                    fmt::format_context>::value,
                "");
}

#if FMT_HAS_FEATURE(cxx_strong_enums)
enum big_enum : unsigned long long { big_enum_value = 5000000000ULL };
auto format_as(big_enum e) -> unsigned long long { return e; }

TEST(format_test, strong_enum) {
  EXPECT_EQ("5000000000", fmt::format("{}", big_enum_value));
}
#endif

TEST(format_test, non_null_terminated_format_string) {
  EXPECT_EQ("42", fmt::format(string_view("{}foo", 2), 42));
}

struct variant {
  enum { int_type, string_type } type;
  explicit variant(int) : type(int_type) {}
  explicit variant(const char*) : type(string_type) {}
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<variant> : dynamic_formatter<> {
  auto format(variant value, format_context& ctx) -> decltype(ctx.out()) {
    if (value.type == variant::int_type)
      return dynamic_formatter<>::format(42, ctx);
    return dynamic_formatter<>::format("foo", ctx);
  }
};
FMT_END_NAMESPACE

TEST(format_test, dynamic_formatter) {
  auto num = variant(42);
  auto str = variant("foo");
  EXPECT_EQ("42", fmt::format("{:d}", num));
  EXPECT_EQ("foo", fmt::format("{:s}", str));
  EXPECT_EQ(" 42 foo ", fmt::format("{:{}} {:{}}", num, 3, str, 4));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:{}}"), num), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:{0}}"), num), format_error,
                   "cannot switch from automatic to manual argument indexing");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:+}"), str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:-}"), str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{: }"), str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:#}"), str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:0}"), str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:.2}"), num), format_error,
                   "precision not allowed for this argument type");
}

namespace adl_test {
namespace fmt {
namespace detail {
struct foo {};
template <typename, typename OutputIt> void write(OutputIt, foo) = delete;
}  // namespace detail
}  // namespace fmt
}  // namespace adl_test

FMT_BEGIN_NAMESPACE
template <>
struct formatter<adl_test::fmt::detail::foo> : formatter<std::string> {
  template <typename FormatContext>
  auto format(adl_test::fmt::detail::foo, FormatContext& ctx)
      -> decltype(ctx.out()) {
    return formatter<std::string>::format("foo", ctx);
  }
};
FMT_END_NAMESPACE

struct convertible_to_int {
  operator int() const { return value; }

  int value = 42;
};

TEST(format_test, to_string) {
  EXPECT_EQ(fmt::to_string(42), "42");
  EXPECT_EQ(fmt::to_string(reinterpret_cast<void*>(0x1234)), "0x1234");
  EXPECT_EQ(fmt::to_string(adl_test::fmt::detail::foo()), "foo");
  EXPECT_EQ(fmt::to_string(convertible_to_int()), "42");
  EXPECT_EQ(fmt::to_string(foo), "0");

#if FMT_USE_FLOAT128
  EXPECT_EQ(fmt::to_string(__float128(0.5)), "0.5");
#endif
}

TEST(format_test, output_iterators) {
  std::list<char> out;
  fmt::format_to(std::back_inserter(out), "{}", 42);
  EXPECT_EQ("42", std::string(out.begin(), out.end()));
  std::stringstream s;
  fmt::format_to(std::ostream_iterator<char>(s), "{}", 42);
  EXPECT_EQ("42", s.str());
}

TEST(format_test, formatted_size) {
  EXPECT_EQ(2u, fmt::formatted_size("{}", 42));
}

TEST(format_test, format_to_no_args) {
  std::string s;
  fmt::format_to(std::back_inserter(s), "test");
  EXPECT_EQ("test", s);
}

TEST(format_test, format_to) {
  std::string s;
  fmt::format_to(std::back_inserter(s), "part{0}", 1);
  EXPECT_EQ("part1", s);
  fmt::format_to(std::back_inserter(s), "part{0}", 2);
  EXPECT_EQ("part1part2", s);
}

TEST(format_test, format_to_memory_buffer) {
  auto buf = fmt::basic_memory_buffer<char, 100>();
  fmt::format_to(fmt::appender(buf), "{}", "foo");
  EXPECT_EQ("foo", to_string(buf));
}

TEST(format_test, format_to_vector) {
  std::vector<char> v;
  fmt::format_to(std::back_inserter(v), "{}", "foo");
  EXPECT_EQ(string_view(v.data(), v.size()), "foo");
}

struct nongrowing_container {
  using value_type = char;
  void push_back(char) { throw std::runtime_error("can't take it any more"); }
};

TEST(format_test, format_to_propagates_exceptions) {
  auto c = nongrowing_container();
  EXPECT_THROW(fmt::format_to(std::back_inserter(c), "{}", 42),
               std::runtime_error);
}

TEST(format_test, format_to_n) {
  char buffer[4];
  buffer[3] = 'x';
  auto result = fmt::format_to_n(buffer, 3, "{}", 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("123x", fmt::string_view(buffer, 4));

  result = fmt::format_to_n(buffer, 3, "{:s}", "foobar");
  EXPECT_EQ(6u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("foox", fmt::string_view(buffer, 4));

  buffer[0] = 'x';
  buffer[1] = 'x';
  buffer[2] = 'x';
  result = fmt::format_to_n(buffer, 3, "{}", 'A');
  EXPECT_EQ(1u, result.size);
  EXPECT_EQ(buffer + 1, result.out);
  EXPECT_EQ("Axxx", fmt::string_view(buffer, 4));

  result = fmt::format_to_n(buffer, 3, "{}{} ", 'B', 'C');
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("BC x", fmt::string_view(buffer, 4));

  result = fmt::format_to_n(buffer, 4, "{}", "ABCDE");
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ("ABCD", fmt::string_view(buffer, 4));

  buffer[3] = 'x';
  result = fmt::format_to_n(buffer, 3, "{}", std::string(1000, '*'));
  EXPECT_EQ(1000u, result.size);
  EXPECT_EQ("***x", fmt::string_view(buffer, 4));
}

struct test_output_iterator {
  char* data;

  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = void;
  using pointer = void;
  using reference = void;

  test_output_iterator& operator++() {
    ++data;
    return *this;
  }
  test_output_iterator operator++(int) {
    auto tmp = *this;
    ++data;
    return tmp;
  }
  char& operator*() { return *data; }
};

TEST(format_test, format_to_n_output_iterator) {
  char buf[10] = {};
  fmt::format_to_n(test_output_iterator{buf}, 10, "{}", 42);
  EXPECT_STREQ(buf, "42");
}

#if FMT_USE_CONSTEXPR
struct test_error_handler {
  const char*& error;

  FMT_CONSTEXPR test_error_handler(const char*& err) : error(err) {}

  FMT_CONSTEXPR test_error_handler(const test_error_handler& other)
      : error(other.error) {}

  FMT_CONSTEXPR void on_error(const char* message) {
    if (!error) error = message;
  }
};

FMT_CONSTEXPR size_t len(const char* s) {
  size_t len = 0;
  while (*s++) ++len;
  return len;
}

FMT_CONSTEXPR bool equal(const char* s1, const char* s2) {
  if (!s1 || !s2) return s1 == s2;
  while (*s1 && *s1 == *s2) {
    ++s1;
    ++s2;
  }
  return *s1 == *s2;
}

template <typename... Args>
FMT_CONSTEXPR bool test_error(const char* fmt, const char* expected_error) {
  const char* actual_error = nullptr;
  auto s = string_view(fmt, len(fmt));
  auto checker =
      fmt::detail::format_string_checker<char, test_error_handler, Args...>(
          s, test_error_handler(actual_error));
  fmt::detail::parse_format_string<true>(s, checker);
  return equal(actual_error, expected_error);
}

#  define EXPECT_ERROR_NOARGS(fmt, error) \
    static_assert(test_error(fmt, error), "")
#  define EXPECT_ERROR(fmt, error, ...) \
    static_assert(test_error<__VA_ARGS__>(fmt, error), "")

TEST(format_test, format_string_errors) {
  EXPECT_ERROR_NOARGS("foo", nullptr);
  EXPECT_ERROR_NOARGS("}", "unmatched '}' in format string");
  EXPECT_ERROR("{0:s", "unknown format specifier", date);
#  if !FMT_MSC_VERSION || FMT_MSC_VERSION >= 1916
  // This causes an detail compiler error in MSVC2017.
  EXPECT_ERROR("{:{<}", "invalid fill character '{'", int);
  EXPECT_ERROR("{:10000000000}", "number is too big", int);
  EXPECT_ERROR("{:.10000000000}", "number is too big", int);
  EXPECT_ERROR_NOARGS("{:x}", "argument not found");
  EXPECT_ERROR("{:+}", "format specifier requires numeric argument",
               const char*);
  EXPECT_ERROR("{:-}", "format specifier requires numeric argument",
               const char*);
  EXPECT_ERROR("{:#}", "format specifier requires numeric argument",
               const char*);
  EXPECT_ERROR("{: }", "format specifier requires numeric argument",
               const char*);
  EXPECT_ERROR("{:0}", "format specifier requires numeric argument",
               const char*);
  EXPECT_ERROR("{:+}", "format specifier requires signed argument", unsigned);
  EXPECT_ERROR("{:-}", "format specifier requires signed argument", unsigned);
  EXPECT_ERROR("{: }", "format specifier requires signed argument", unsigned);
  EXPECT_ERROR("{:{}}", "argument not found", int);
  EXPECT_ERROR("{:.{}}", "argument not found", double);
  EXPECT_ERROR("{:{}}", "width/precision is not integer", int, double);
  EXPECT_ERROR("{:.2}", "precision not allowed for this argument type", int);
  EXPECT_ERROR("{:s}", "invalid type specifier", int);
  EXPECT_ERROR("{:s}", "invalid type specifier", char);
  EXPECT_ERROR("{:+}", "invalid format specifier for char", char);
  EXPECT_ERROR("{:s}", "invalid type specifier", double);
  EXPECT_ERROR("{:d}", "invalid type specifier", const char*);
  EXPECT_ERROR("{:d}", "invalid type specifier", std::string);
  EXPECT_ERROR("{:s}", "invalid type specifier", void*);
#  else
  fmt::print("warning: constexpr is broken in this version of MSVC\n");
#  endif
#  if FMT_USE_NONTYPE_TEMPLATE_ARGS
  using namespace fmt::literals;
  EXPECT_ERROR("{foo}", "named argument is not found", decltype("bar"_a = 42));
  EXPECT_ERROR("{foo}", "named argument is not found",
               decltype(fmt::arg("foo", 42)));
#  else
  EXPECT_ERROR("{foo}",
               "compile-time checks for named arguments require C++20 support",
               int);
#  endif
  EXPECT_ERROR_NOARGS("{10000000000}", "argument not found");
  EXPECT_ERROR_NOARGS("{0x}", "invalid format string");
  EXPECT_ERROR_NOARGS("{-}", "invalid format string");
  EXPECT_ERROR("{:{0x}}", "invalid format string", int);
  EXPECT_ERROR("{:{-}}", "invalid format string", int);
  EXPECT_ERROR("{:.{0x}}", "invalid format string", int);
  EXPECT_ERROR("{:.{-}}", "invalid format string", int);
  EXPECT_ERROR("{:.x}", "missing precision specifier", int);
  EXPECT_ERROR_NOARGS("{}", "argument not found");
  EXPECT_ERROR("{1}", "argument not found", int);
  EXPECT_ERROR("{1}{}",
               "cannot switch from manual to automatic argument indexing", int,
               int);
  EXPECT_ERROR("{}{1}",
               "cannot switch from automatic to manual argument indexing", int,
               int);
}

TEST(format_test, vformat_to) {
  using context = fmt::format_context;
  fmt::basic_format_arg<context> arg = fmt::detail::make_arg<context>(42);
  auto args = fmt::basic_format_args<context>(&arg, 1);
  auto s = std::string();
  fmt::vformat_to(std::back_inserter(s), "{}", args);
  EXPECT_EQ("42", s);
  s.clear();
  fmt::vformat_to(std::back_inserter(s), FMT_STRING("{}"), args);
  EXPECT_EQ("42", s);
}

#endif  // FMT_USE_CONSTEXPR

TEST(format_test, char_traits_is_not_ambiguous) {
  // Test that we don't inject detail names into the std namespace.
  using namespace std;
  auto c = char_traits<char>::char_type();
  (void)c;
#if FMT_CPLUSPLUS >= 201103L
  auto s = std::string();
  auto lval = begin(s);
  (void)lval;
#endif
}

struct check_back_appender {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<check_back_appender> {
  auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename Context>
  auto format(check_back_appender, Context& ctx) -> decltype(ctx.out()) {
    auto out = ctx.out();
    static_assert(std::is_same<decltype(++out), decltype(out)&>::value,
                  "needs to satisfy weakly_incrementable");
    *out = 'y';
    return ++out;
  }
};
FMT_END_NAMESPACE

TEST(format_test, back_insert_slicing) {
  EXPECT_EQ(fmt::format("{}", check_back_appender{}), "y");
}

template <typename Char, typename T> bool check_enabled_formatter() {
  static_assert(std::is_default_constructible<fmt::formatter<T, Char>>::value,
                "");
  return true;
}

template <typename Char, typename... T> void check_enabled_formatters() {
  auto dummy = {check_enabled_formatter<Char, T>()...};
  (void)dummy;
}

TEST(format_test, test_formatters_enabled) {
  check_enabled_formatters<char, bool, char, signed char, unsigned char, short,
                           unsigned short, int, unsigned, long, unsigned long,
                           long long, unsigned long long, float, double,
                           long double, void*, const void*, char*, const char*,
                           std::string, std::nullptr_t>();
  check_enabled_formatters<wchar_t, bool, wchar_t, signed char, unsigned char,
                           short, unsigned short, int, unsigned, long,
                           unsigned long, long long, unsigned long long, float,
                           double, long double, void*, const void*, wchar_t*,
                           const wchar_t*, std::wstring, std::nullptr_t>();
}

TEST(format_int_test, data) {
  fmt::format_int format_int(42);
  EXPECT_EQ("42", std::string(format_int.data(), format_int.size()));
}

TEST(format_int_test, format_int) {
  EXPECT_EQ("42", fmt::format_int(42).str());
  EXPECT_EQ(2u, fmt::format_int(42).size());
  EXPECT_EQ("-42", fmt::format_int(-42).str());
  EXPECT_EQ(3u, fmt::format_int(-42).size());
  EXPECT_EQ("42", fmt::format_int(42ul).str());
  EXPECT_EQ("-42", fmt::format_int(-42l).str());
  EXPECT_EQ("42", fmt::format_int(42ull).str());
  EXPECT_EQ("-42", fmt::format_int(-42ll).str());
  std::ostringstream os;
  os << max_value<int64_t>();
  EXPECT_EQ(os.str(), fmt::format_int(max_value<int64_t>()).str());
}
