// Formatting library for C++ - core tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

// clang-format off
#include "test-assert.h"
// clang-format on

#define I 42  // simulate https://en.cppreference.com/w/c/numeric/complex/I
#include "fmt/core.h"
#undef I

#include <algorithm>    // std::copy_n
#include <climits>      // INT_MAX
#include <cstring>      // std::strlen
#include <functional>   // std::equal_to
#include <iterator>     // std::back_insert_iterator
#include <limits>       // std::numeric_limits
#include <string>       // std::string
#include <type_traits>  // std::is_same

#include "gmock/gmock.h"

using fmt::string_view;
using fmt::detail::buffer;

using testing::_;
using testing::Invoke;
using testing::Return;

#ifdef FMT_FORMAT_H_
#  error core-test includes format.h
#endif

TEST(string_view_test, value_type) {
  static_assert(std::is_same<string_view::value_type, char>::value, "");
}

TEST(string_view_test, ctor) {
  EXPECT_STREQ("abc", fmt::string_view("abc").data());
  EXPECT_EQ(3u, fmt::string_view("abc").size());

  EXPECT_STREQ("defg", fmt::string_view(std::string("defg")).data());
  EXPECT_EQ(4u, fmt::string_view(std::string("defg")).size());
}

TEST(string_view_test, length) {
  // Test that string_view::size() returns string length, not buffer size.
  char str[100] = "some string";
  EXPECT_EQ(std::strlen(str), string_view(str).size());
  EXPECT_LT(std::strlen(str), sizeof(str));
}

// Check string_view's comparison operator.
template <template <typename> class Op> void check_op() {
  const char* inputs[] = {"foo", "fop", "fo"};
  size_t num_inputs = sizeof(inputs) / sizeof(*inputs);
  for (size_t i = 0; i < num_inputs; ++i) {
    for (size_t j = 0; j < num_inputs; ++j) {
      string_view lhs(inputs[i]), rhs(inputs[j]);
      EXPECT_EQ(Op<int>()(lhs.compare(rhs), 0), Op<string_view>()(lhs, rhs));
    }
  }
}

TEST(string_view_test, compare) {
  EXPECT_EQ(string_view("foo").compare(string_view("foo")), 0);
  EXPECT_GT(string_view("fop").compare(string_view("foo")), 0);
  EXPECT_LT(string_view("foo").compare(string_view("fop")), 0);
  EXPECT_GT(string_view("foo").compare(string_view("fo")), 0);
  EXPECT_LT(string_view("fo").compare(string_view("foo")), 0);
  check_op<std::equal_to>();
  check_op<std::not_equal_to>();
  check_op<std::less>();
  check_op<std::less_equal>();
  check_op<std::greater>();
  check_op<std::greater_equal>();
}

namespace test_ns {
template <typename Char> class test_string {
 private:
  std::basic_string<Char> s_;

 public:
  test_string(const Char* s) : s_(s) {}
  const Char* data() const { return s_.data(); }
  size_t length() const { return s_.size(); }
  operator const Char*() const { return s_.c_str(); }
};

template <typename Char>
fmt::basic_string_view<Char> to_string_view(const test_string<Char>& s) {
  return {s.data(), s.length()};
}
}  // namespace test_ns

TEST(core_test, is_output_iterator) {
  EXPECT_TRUE((fmt::detail::is_output_iterator<char*, char>::value));
  EXPECT_FALSE((fmt::detail::is_output_iterator<const char*, char>::value));
  EXPECT_FALSE((fmt::detail::is_output_iterator<std::string, char>::value));
  EXPECT_TRUE(
      (fmt::detail::is_output_iterator<std::back_insert_iterator<std::string>,
                                       char>::value));
  EXPECT_TRUE(
      (fmt::detail::is_output_iterator<std::string::iterator, char>::value));
  EXPECT_FALSE((fmt::detail::is_output_iterator<std::string::const_iterator,
                                                char>::value));
}

TEST(core_test, buffer_appender) {
  // back_insert_iterator is not default-constructible before C++20, so
  // buffer_appender can only be default-constructible when back_insert_iterator
  // is.
  static_assert(
      std::is_default_constructible<
          std::back_insert_iterator<fmt::detail::buffer<char>>>::value ==
          std::is_default_constructible<
              fmt::detail::buffer_appender<char>>::value,
      "");

#ifdef __cpp_lib_ranges
  static_assert(std::output_iterator<fmt::detail::buffer_appender<char>, char>);
#endif
}

#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 470
TEST(buffer_test, noncopyable) {
  EXPECT_FALSE(std::is_copy_constructible<buffer<char>>::value);
#  if !FMT_MSC_VERSION
  // std::is_copy_assignable is broken in MSVC2013.
  EXPECT_FALSE(std::is_copy_assignable<buffer<char>>::value);
#  endif
}

TEST(buffer_test, nonmoveable) {
  EXPECT_FALSE(std::is_move_constructible<buffer<char>>::value);
#  if !FMT_MSC_VERSION
  // std::is_move_assignable is broken in MSVC2013.
  EXPECT_FALSE(std::is_move_assignable<buffer<char>>::value);
#  endif
}
#endif

TEST(buffer_test, indestructible) {
  static_assert(!std::is_destructible<fmt::detail::buffer<int>>(),
                "buffer's destructor is protected");
}

template <typename T> struct mock_buffer final : buffer<T> {
  MOCK_METHOD1(do_grow, size_t(size_t capacity));

  void grow(size_t capacity) override {
    this->set(this->data(), do_grow(capacity));
  }

  mock_buffer(T* data = nullptr, size_t buf_capacity = 0) {
    this->set(data, buf_capacity);
    ON_CALL(*this, do_grow(_)).WillByDefault(Invoke([](size_t capacity) {
      return capacity;
    }));
  }
};

TEST(buffer_test, ctor) {
  {
    mock_buffer<int> buffer;
    EXPECT_EQ(nullptr, buffer.data());
    EXPECT_EQ(static_cast<size_t>(0), buffer.size());
    EXPECT_EQ(static_cast<size_t>(0), buffer.capacity());
  }
  {
    int dummy;
    mock_buffer<int> buffer(&dummy);
    EXPECT_EQ(&dummy, &buffer[0]);
    EXPECT_EQ(static_cast<size_t>(0), buffer.size());
    EXPECT_EQ(static_cast<size_t>(0), buffer.capacity());
  }
  {
    int dummy;
    size_t capacity = std::numeric_limits<size_t>::max();
    mock_buffer<int> buffer(&dummy, capacity);
    EXPECT_EQ(&dummy, &buffer[0]);
    EXPECT_EQ(static_cast<size_t>(0), buffer.size());
    EXPECT_EQ(capacity, buffer.capacity());
  }
}

TEST(buffer_test, access) {
  char data[10];
  mock_buffer<char> buffer(data, sizeof(data));
  buffer[0] = 11;
  EXPECT_EQ(11, buffer[0]);
  buffer[3] = 42;
  EXPECT_EQ(42, *(&buffer[0] + 3));
  const fmt::detail::buffer<char>& const_buffer = buffer;
  EXPECT_EQ(42, const_buffer[3]);
}

TEST(buffer_test, try_resize) {
  char data[123];
  mock_buffer<char> buffer(data, sizeof(data));
  buffer[10] = 42;
  EXPECT_EQ(42, buffer[10]);
  buffer.try_resize(20);
  EXPECT_EQ(20u, buffer.size());
  EXPECT_EQ(123u, buffer.capacity());
  EXPECT_EQ(42, buffer[10]);
  buffer.try_resize(5);
  EXPECT_EQ(5u, buffer.size());
  EXPECT_EQ(123u, buffer.capacity());
  EXPECT_EQ(42, buffer[10]);
  // Check if try_resize calls grow.
  EXPECT_CALL(buffer, do_grow(124));
  buffer.try_resize(124);
  EXPECT_CALL(buffer, do_grow(200));
  buffer.try_resize(200);
}

TEST(buffer_test, try_resize_partial) {
  char data[10];
  mock_buffer<char> buffer(data, sizeof(data));
  EXPECT_CALL(buffer, do_grow(20)).WillOnce(Return(15));
  buffer.try_resize(20);
  EXPECT_EQ(buffer.capacity(), 15);
  EXPECT_EQ(buffer.size(), 15);
}

TEST(buffer_test, clear) {
  mock_buffer<char> buffer;
  EXPECT_CALL(buffer, do_grow(20));
  buffer.try_resize(20);
  buffer.try_resize(0);
  EXPECT_EQ(static_cast<size_t>(0), buffer.size());
  EXPECT_EQ(20u, buffer.capacity());
}

TEST(buffer_test, append) {
  char data[15];
  mock_buffer<char> buffer(data, 10);
  auto test = "test";
  buffer.append(test, test + 5);
  EXPECT_STREQ(test, &buffer[0]);
  EXPECT_EQ(5u, buffer.size());
  buffer.try_resize(10);
  EXPECT_CALL(buffer, do_grow(12));
  buffer.append(test, test + 2);
  EXPECT_EQ('t', buffer[10]);
  EXPECT_EQ('e', buffer[11]);
  EXPECT_EQ(12u, buffer.size());
}

TEST(buffer_test, append_partial) {
  char data[10];
  mock_buffer<char> buffer(data, sizeof(data));
  testing::InSequence seq;
  EXPECT_CALL(buffer, do_grow(15)).WillOnce(Return(10));
  EXPECT_CALL(buffer, do_grow(15)).WillOnce(Invoke([&buffer](size_t) {
    EXPECT_EQ(fmt::string_view(buffer.data(), buffer.size()), "0123456789");
    buffer.clear();
    return 10;
  }));
  auto test = "0123456789abcde";
  buffer.append(test, test + 15);
}

TEST(buffer_test, append_allocates_enough_storage) {
  char data[19];
  mock_buffer<char> buffer(data, 10);
  auto test = "abcdefgh";
  buffer.try_resize(10);
  EXPECT_CALL(buffer, do_grow(19));
  buffer.append(test, test + 9);
}

struct custom_context {
  using char_type = char;
  using parse_context_type = fmt::format_parse_context;

  bool called = false;

  template <typename T> struct formatter_type {
    auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
      return ctx.begin();
    }

    const char* format(const T&, custom_context& ctx) {
      ctx.called = true;
      return nullptr;
    }
  };

  void advance_to(const char*) {}
};

struct test_struct {};

FMT_BEGIN_NAMESPACE
template <typename Char> struct formatter<test_struct, Char> {
  auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  auto format(test_struct, format_context& ctx) -> decltype(ctx.out()) {
    auto test = string_view("test");
    return std::copy_n(test.data(), test.size(), ctx.out());
  }
};
FMT_END_NAMESPACE

TEST(arg_test, format_args) {
  auto args = fmt::format_args();
  EXPECT_FALSE(args.get(1));
}

TEST(arg_test, make_value_with_custom_context) {
  auto t = test_struct();
  fmt::detail::value<custom_context> arg(
      fmt::detail::arg_mapper<custom_context>().map(t));
  auto ctx = custom_context();
  auto parse_ctx = fmt::format_parse_context("");
  arg.custom.format(&t, parse_ctx, ctx);
  EXPECT_TRUE(ctx.called);
}

// Use a unique result type to make sure that there are no undesirable
// conversions.
struct test_result {};

template <typename T> struct mock_visitor {
  template <typename U> struct result { using type = test_result; };

  mock_visitor() {
    ON_CALL(*this, visit(_)).WillByDefault(Return(test_result()));
  }

  MOCK_METHOD1_T(visit, test_result(T value));
  MOCK_METHOD0_T(unexpected, void());

  test_result operator()(T value) { return visit(value); }

  template <typename U> test_result operator()(U) {
    unexpected();
    return test_result();
  }
};

template <typename T> struct visit_type { using type = T; };

#define VISIT_TYPE(type_, visit_type_) \
  template <> struct visit_type<type_> { using type = visit_type_; }

VISIT_TYPE(signed char, int);
VISIT_TYPE(unsigned char, unsigned);
VISIT_TYPE(short, int);
VISIT_TYPE(unsigned short, unsigned);

#if LONG_MAX == INT_MAX
VISIT_TYPE(long, int);
VISIT_TYPE(unsigned long, unsigned);
#else
VISIT_TYPE(long, long long);
VISIT_TYPE(unsigned long, unsigned long long);
#endif

#define CHECK_ARG(Char, expected, value)                                  \
  {                                                                       \
    testing::StrictMock<mock_visitor<decltype(expected)>> visitor;        \
    EXPECT_CALL(visitor, visit(expected));                                \
    using iterator = std::back_insert_iterator<buffer<Char>>;             \
    fmt::visit_format_arg(                                                \
        visitor,                                                          \
        fmt::detail::make_arg<fmt::basic_format_context<iterator, Char>>( \
            value));                                                      \
  }

#define CHECK_ARG_SIMPLE(value)                             \
  {                                                         \
    using value_type = decltype(value);                     \
    typename visit_type<value_type>::type expected = value; \
    CHECK_ARG(char, expected, value)                        \
    CHECK_ARG(wchar_t, expected, value)                     \
  }

template <typename T> class numeric_arg_test : public testing::Test {};

using test_types =
    testing::Types<bool, signed char, unsigned char, short, unsigned short, int,
                   unsigned, long, unsigned long, long long, unsigned long long,
                   float, double, long double>;
TYPED_TEST_SUITE(numeric_arg_test, test_types);

template <typename T, fmt::enable_if_t<std::is_integral<T>::value, int> = 0>
T test_value() {
  return static_cast<T>(42);
}

template <typename T,
          fmt::enable_if_t<std::is_floating_point<T>::value, int> = 0>
T test_value() {
  return static_cast<T>(4.2);
}

TYPED_TEST(numeric_arg_test, make_and_visit) {
  CHECK_ARG_SIMPLE(test_value<TypeParam>());
  CHECK_ARG_SIMPLE(std::numeric_limits<TypeParam>::min());
  CHECK_ARG_SIMPLE(std::numeric_limits<TypeParam>::max());
}

TEST(arg_test, char_arg) { CHECK_ARG(char, 'a', 'a'); }

TEST(arg_test, string_arg) {
  char str_data[] = "test";
  char* str = str_data;
  const char* cstr = str;
  CHECK_ARG(char, cstr, str);

  auto sv = fmt::string_view(str);
  CHECK_ARG(char, sv, std::string(str));
}

TEST(arg_test, wstring_arg) {
  wchar_t str_data[] = L"test";
  wchar_t* str = str_data;
  const wchar_t* cstr = str;

  auto sv = fmt::basic_string_view<wchar_t>(str);
  CHECK_ARG(wchar_t, cstr, str);
  CHECK_ARG(wchar_t, cstr, cstr);
  CHECK_ARG(wchar_t, sv, std::wstring(str));
  CHECK_ARG(wchar_t, sv, fmt::basic_string_view<wchar_t>(str));
}

TEST(arg_test, pointer_arg) {
  void* p = nullptr;
  const void* cp = nullptr;
  CHECK_ARG(char, cp, p);
  CHECK_ARG(wchar_t, cp, p);
  CHECK_ARG_SIMPLE(cp);
}

struct check_custom {
  test_result operator()(
      fmt::basic_format_arg<fmt::format_context>::handle h) const {
    struct test_buffer final : fmt::detail::buffer<char> {
      char data[10];
      test_buffer() : fmt::detail::buffer<char>(data, 0, 10) {}
      void grow(size_t) override {}
    } buffer;
    auto parse_ctx = fmt::format_parse_context("");
    auto ctx = fmt::format_context(fmt::detail::buffer_appender<char>(buffer),
                                   fmt::format_args());
    h.format(parse_ctx, ctx);
    EXPECT_EQ("test", std::string(buffer.data, buffer.size()));
    return test_result();
  }
};

TEST(arg_test, custom_arg) {
  auto test = test_struct();
  using visitor =
      mock_visitor<fmt::basic_format_arg<fmt::format_context>::handle>;
  testing::StrictMock<visitor> v;
  EXPECT_CALL(v, visit(_)).WillOnce(Invoke(check_custom()));
  fmt::visit_format_arg(v, fmt::detail::make_arg<fmt::format_context>(test));
}

TEST(arg_test, visit_invalid_arg) {
  testing::StrictMock<mock_visitor<fmt::monostate>> visitor;
  EXPECT_CALL(visitor, visit(_));
  auto arg = fmt::basic_format_arg<fmt::format_context>();
  fmt::visit_format_arg(visitor, arg);
}

#if FMT_USE_CONSTEXPR

enum class arg_id_result { none, empty, index, name, error };
struct test_arg_id_handler {
  arg_id_result res = arg_id_result::none;
  int index = 0;
  string_view name;

  constexpr void operator()() { res = arg_id_result::empty; }

  constexpr void operator()(int i) {
    res = arg_id_result::index;
    index = i;
  }

  constexpr void operator()(string_view n) {
    res = arg_id_result::name;
    name = n;
  }

  constexpr void on_error(const char*) { res = arg_id_result::error; }
};

template <size_t N>
constexpr test_arg_id_handler parse_arg_id(const char (&s)[N]) {
  test_arg_id_handler h;
  fmt::detail::parse_arg_id(s, s + N, h);
  return h;
}

TEST(format_test, constexpr_parse_arg_id) {
  static_assert(parse_arg_id(":").res == arg_id_result::empty, "");
  static_assert(parse_arg_id("}").res == arg_id_result::empty, "");
  static_assert(parse_arg_id("42:").res == arg_id_result::index, "");
  static_assert(parse_arg_id("42:").index == 42, "");
  static_assert(parse_arg_id("foo:").res == arg_id_result::name, "");
  static_assert(parse_arg_id("foo:").name.size() == 3, "");
  static_assert(parse_arg_id("!").res == arg_id_result::error, "");
}

struct test_format_specs_handler {
  enum result { none, hash, zero, loc, error };
  result res = none;

  fmt::align_t alignment = fmt::align::none;
  fmt::sign_t sign = fmt::sign::none;
  char fill = 0;
  int width = 0;
  fmt::detail::arg_ref<char> width_ref;
  int precision = 0;
  fmt::detail::arg_ref<char> precision_ref;
  fmt::presentation_type type = fmt::presentation_type::none;

  // Workaround for MSVC2017 bug that results in "expression did not evaluate
  // to a constant" with compiler-generated copy ctor.
  constexpr test_format_specs_handler() {}
  constexpr test_format_specs_handler(const test_format_specs_handler& other) =
      default;

  constexpr void on_align(fmt::align_t a) { alignment = a; }
  constexpr void on_fill(fmt::string_view f) { fill = f[0]; }
  constexpr void on_sign(fmt::sign_t s) { sign = s; }
  constexpr void on_hash() { res = hash; }
  constexpr void on_zero() { res = zero; }
  constexpr void on_localized() { res = loc; }

  constexpr void on_width(int w) { width = w; }
  constexpr void on_dynamic_width(fmt::detail::auto_id) {}
  constexpr void on_dynamic_width(int index) { width_ref = index; }
  constexpr void on_dynamic_width(string_view) {}

  constexpr void on_precision(int p) { precision = p; }
  constexpr void on_dynamic_precision(fmt::detail::auto_id) {}
  constexpr void on_dynamic_precision(int index) { precision_ref = index; }
  constexpr void on_dynamic_precision(string_view) {}

  constexpr void end_precision() {}
  constexpr void on_type(fmt::presentation_type t) { type = t; }
  constexpr void on_error(const char*) { res = error; }
};

template <size_t N>
constexpr test_format_specs_handler parse_test_specs(const char (&s)[N]) {
  auto h = test_format_specs_handler();
  fmt::detail::parse_format_specs(s, s + N - 1, h);
  return h;
}

TEST(core_test, constexpr_parse_format_specs) {
  using handler = test_format_specs_handler;
  static_assert(parse_test_specs("<").alignment == fmt::align::left, "");
  static_assert(parse_test_specs("*^").fill == '*', "");
  static_assert(parse_test_specs("+").sign == fmt::sign::plus, "");
  static_assert(parse_test_specs("-").sign == fmt::sign::minus, "");
  static_assert(parse_test_specs(" ").sign == fmt::sign::space, "");
  static_assert(parse_test_specs("#").res == handler::hash, "");
  static_assert(parse_test_specs("0").res == handler::zero, "");
  static_assert(parse_test_specs("L").res == handler::loc, "");
  static_assert(parse_test_specs("42").width == 42, "");
  static_assert(parse_test_specs("{42}").width_ref.val.index == 42, "");
  static_assert(parse_test_specs(".42").precision == 42, "");
  static_assert(parse_test_specs(".{42}").precision_ref.val.index == 42, "");
  static_assert(parse_test_specs("d").type == fmt::presentation_type::dec, "");
  static_assert(parse_test_specs("{<").res == handler::error, "");
}

struct test_parse_context {
  using char_type = char;

  constexpr int next_arg_id() { return 11; }
  template <typename Id> FMT_CONSTEXPR void check_arg_id(Id) {}
  FMT_CONSTEXPR void check_dynamic_spec(int) {}

  constexpr const char* begin() { return nullptr; }
  constexpr const char* end() { return nullptr; }

  void on_error(const char*) {}
};

template <size_t N>
constexpr fmt::detail::dynamic_format_specs<char> parse_dynamic_specs(
    const char (&s)[N]) {
  auto specs = fmt::detail::dynamic_format_specs<char>();
  auto ctx = test_parse_context();
  auto h = fmt::detail::dynamic_specs_handler<test_parse_context>(specs, ctx);
  parse_format_specs(s, s + N - 1, h);
  return specs;
}

TEST(format_test, constexpr_dynamic_specs_handler) {
  static_assert(parse_dynamic_specs("<").align == fmt::align::left, "");
  static_assert(parse_dynamic_specs("*^").fill[0] == '*', "");
  static_assert(parse_dynamic_specs("+").sign == fmt::sign::plus, "");
  static_assert(parse_dynamic_specs("-").sign == fmt::sign::minus, "");
  static_assert(parse_dynamic_specs(" ").sign == fmt::sign::space, "");
  static_assert(parse_dynamic_specs("#").alt, "");
  static_assert(parse_dynamic_specs("0").align == fmt::align::numeric, "");
  static_assert(parse_dynamic_specs("42").width == 42, "");
  static_assert(parse_dynamic_specs("{}").width_ref.val.index == 11, "");
  static_assert(parse_dynamic_specs("{42}").width_ref.val.index == 42, "");
  static_assert(parse_dynamic_specs(".42").precision == 42, "");
  static_assert(parse_dynamic_specs(".{}").precision_ref.val.index == 11, "");
  static_assert(parse_dynamic_specs(".{42}").precision_ref.val.index == 42, "");
  static_assert(parse_dynamic_specs("d").type == fmt::presentation_type::dec,
                "");
}

template <size_t N>
constexpr test_format_specs_handler check_specs(const char (&s)[N]) {
  fmt::detail::specs_checker<test_format_specs_handler> checker(
      test_format_specs_handler(), fmt::detail::type::double_type);
  parse_format_specs(s, s + N - 1, checker);
  return checker;
}

TEST(format_test, constexpr_specs_checker) {
  using handler = test_format_specs_handler;
  static_assert(check_specs("<").alignment == fmt::align::left, "");
  static_assert(check_specs("*^").fill == '*', "");
  static_assert(check_specs("+").sign == fmt::sign::plus, "");
  static_assert(check_specs("-").sign == fmt::sign::minus, "");
  static_assert(check_specs(" ").sign == fmt::sign::space, "");
  static_assert(check_specs("#").res == handler::hash, "");
  static_assert(check_specs("0").res == handler::zero, "");
  static_assert(check_specs("42").width == 42, "");
  static_assert(check_specs("{42}").width_ref.val.index == 42, "");
  static_assert(check_specs(".42").precision == 42, "");
  static_assert(check_specs(".{42}").precision_ref.val.index == 42, "");
  static_assert(check_specs("d").type == fmt::presentation_type::dec, "");
  static_assert(check_specs("{<").res == handler::error, "");
}

struct test_format_string_handler {
  constexpr void on_text(const char*, const char*) {}

  constexpr int on_arg_id() { return 0; }

  template <typename T> constexpr int on_arg_id(T) { return 0; }

  constexpr void on_replacement_field(int, const char*) {}

  constexpr const char* on_format_specs(int, const char* begin, const char*) {
    return begin;
  }

  constexpr void on_error(const char*) { error = true; }

  bool error = false;
};

template <size_t N> constexpr bool parse_string(const char (&s)[N]) {
  auto h = test_format_string_handler();
  fmt::detail::parse_format_string<true>(fmt::string_view(s, N - 1), h);
  return !h.error;
}

TEST(format_test, constexpr_parse_format_string) {
  static_assert(parse_string("foo"), "");
  static_assert(!parse_string("}"), "");
  static_assert(parse_string("{}"), "");
  static_assert(parse_string("{42}"), "");
  static_assert(parse_string("{foo}"), "");
  static_assert(parse_string("{:}"), "");
}
#endif  // FMT_USE_CONSTEXPR

struct enabled_formatter {};
struct enabled_ptr_formatter {};
struct disabled_formatter {};
struct disabled_formatter_convertible {
  operator int() const { return 42; }
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<enabled_formatter> {
  auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }
  auto format(enabled_formatter, format_context& ctx) -> decltype(ctx.out()) {
    return ctx.out();
  }
};

template <> struct formatter<enabled_ptr_formatter*> {
  auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }
  auto format(enabled_ptr_formatter*, format_context& ctx)
      -> decltype(ctx.out()) {
    return ctx.out();
  }
};
FMT_END_NAMESPACE

TEST(core_test, has_formatter) {
  using fmt::has_formatter;
  using context = fmt::format_context;
  static_assert(has_formatter<enabled_formatter, context>::value, "");
  static_assert(!has_formatter<disabled_formatter, context>::value, "");
  static_assert(!has_formatter<disabled_formatter_convertible, context>::value,
                "");
}

struct const_formattable {};
struct nonconst_formattable {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<const_formattable> {
  auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  auto format(const const_formattable&, format_context& ctx)
      -> decltype(ctx.out()) {
    auto test = string_view("test");
    return std::copy_n(test.data(), test.size(), ctx.out());
  }
};

template <> struct formatter<nonconst_formattable> {
  auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  auto format(nonconst_formattable&, format_context& ctx)
      -> decltype(ctx.out()) {
    auto test = string_view("test");
    return std::copy_n(test.data(), test.size(), ctx.out());
  }
};
FMT_END_NAMESPACE

struct convertible_to_pointer {
  operator const int*() const { return nullptr; }
};

struct convertible_to_pointer_formattable {
  operator const int*() const { return nullptr; }
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<convertible_to_pointer_formattable> {
  auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  auto format(convertible_to_pointer_formattable, format_context& ctx) const
      -> decltype(ctx.out()) {
    auto test = string_view("test");
    return std::copy_n(test.data(), test.size(), ctx.out());
  }
};
FMT_END_NAMESPACE

enum class unformattable_scoped_enum {};

namespace test {
enum class formattable_scoped_enum {};
auto format_as(formattable_scoped_enum) -> int { return 42; }

struct convertible_to_enum {
  operator formattable_scoped_enum() const { return {}; }
};
}  // namespace test

TEST(core_test, is_formattable) {
#if 0
  // This should be enabled once corresponding map overloads are gone.
  static_assert(fmt::is_formattable<signed char*>::value, "");
  static_assert(fmt::is_formattable<unsigned char*>::value, "");
  static_assert(fmt::is_formattable<const signed char*>::value, "");
  static_assert(fmt::is_formattable<const unsigned char*>::value, "");
#endif
  static_assert(!fmt::is_formattable<wchar_t>::value, "");
#ifdef __cpp_char8_t
  static_assert(!fmt::is_formattable<char8_t>::value, "");
#endif
  static_assert(!fmt::is_formattable<char16_t>::value, "");
  static_assert(!fmt::is_formattable<char32_t>::value, "");
  static_assert(!fmt::is_formattable<const wchar_t*>::value, "");
  static_assert(!fmt::is_formattable<const wchar_t[3]>::value, "");
  static_assert(!fmt::is_formattable<fmt::basic_string_view<wchar_t>>::value,
                "");
  static_assert(fmt::is_formattable<enabled_formatter>::value, "");
  static_assert(!fmt::is_formattable<enabled_ptr_formatter*>::value, "");
  static_assert(!fmt::is_formattable<disabled_formatter>::value, "");
  static_assert(fmt::is_formattable<disabled_formatter_convertible>::value, "");

  static_assert(fmt::is_formattable<const_formattable&>::value, "");
  static_assert(fmt::is_formattable<const const_formattable&>::value, "");

  static_assert(fmt::is_formattable<nonconst_formattable&>::value, "");
#if !FMT_MSC_VERSION || FMT_MSC_VERSION >= 1910
  static_assert(!fmt::is_formattable<const nonconst_formattable&>::value, "");
#endif

  static_assert(!fmt::is_formattable<convertible_to_pointer>::value, "");
  const auto f = convertible_to_pointer_formattable();
  EXPECT_EQ(fmt::format("{}", f), "test");

  static_assert(!fmt::is_formattable<void (*)()>::value, "");

  struct s;
  static_assert(!fmt::is_formattable<int(s::*)>::value, "");
  static_assert(!fmt::is_formattable<int (s::*)()>::value, "");
  static_assert(!fmt::is_formattable<unformattable_scoped_enum>::value, "");
  static_assert(fmt::is_formattable<test::formattable_scoped_enum>::value, "");
  static_assert(!fmt::is_formattable<test::convertible_to_enum>::value, "");
}

TEST(core_test, format) { EXPECT_EQ(fmt::format("{}", 42), "42"); }

TEST(core_test, format_to) {
  std::string s;
  fmt::format_to(std::back_inserter(s), "{}", 42);
  EXPECT_EQ(s, "42");
}

TEST(core_test, format_as) {
  EXPECT_EQ(fmt::format("{}", test::formattable_scoped_enum()), "42");
}

struct convertible_to_int {
  operator int() const { return 42; }
};

struct convertible_to_c_string {
  operator const char*() const { return "foo"; }
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<convertible_to_int> {
  auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }
  auto format(convertible_to_int, format_context& ctx) -> decltype(ctx.out()) {
    return std::copy_n("foo", 3, ctx.out());
  }
};

template <> struct formatter<convertible_to_c_string> {
  FMT_CONSTEXPR auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }
  auto format(convertible_to_c_string, format_context& ctx)
      -> decltype(ctx.out()) {
    return std::copy_n("bar", 3, ctx.out());
  }
};
FMT_END_NAMESPACE

TEST(core_test, formatter_overrides_implicit_conversion) {
  EXPECT_EQ(fmt::format("{}", convertible_to_int()), "foo");
  EXPECT_EQ(fmt::format("{}", convertible_to_c_string()), "bar");
}

// Test that check is not found by ADL.
template <typename T> void check(T);
TEST(core_test, adl_check) {
  EXPECT_EQ(fmt::format("{}", test_struct()), "test");
}

TEST(core_test, to_string_view_foreign_strings) {
  using namespace test_ns;
  EXPECT_EQ(to_string_view(test_string<char>("42")), "42");
  fmt::detail::type type =
      fmt::detail::mapped_type_constant<test_string<char>,
                                        fmt::format_context>::value;
  EXPECT_EQ(type, fmt::detail::type::string_type);
}

struct implicitly_convertible_to_string {
  operator std::string() const { return "foo"; }
};

struct implicitly_convertible_to_string_view {
  operator fmt::string_view() const { return "foo"; }
};

TEST(core_test, format_implicitly_convertible_to_string_view) {
  EXPECT_EQ("foo", fmt::format("{}", implicitly_convertible_to_string_view()));
}

// std::is_constructible is broken in MSVC until version 2015.
#if !FMT_MSC_VERSION || FMT_MSC_VERSION >= 1900
struct explicitly_convertible_to_string_view {
  explicit operator fmt::string_view() const { return "foo"; }
};

TEST(core_test, format_explicitly_convertible_to_string_view) {
  // Types explicitly convertible to string_view are not formattable by
  // default because it may introduce ODR violations.
  static_assert(
      !fmt::is_formattable<explicitly_convertible_to_string_view>::value, "");
}

#  ifdef FMT_USE_STRING_VIEW
struct explicitly_convertible_to_std_string_view {
  explicit operator std::string_view() const { return "foo"; }
};

TEST(core_test, format_explicitly_convertible_to_std_string_view) {
  // Types explicitly convertible to string_view are not formattable by
  // default because it may introduce ODR violations.
  static_assert(
      !fmt::is_formattable<explicitly_convertible_to_std_string_view>::value,
      "");
}
#  endif
#endif

struct convertible_to_long_long {
  operator long long() const { return 1LL << 32; }
};

TEST(format_test, format_convertible_to_long_long) {
  EXPECT_EQ("100000000", fmt::format("{:x}", convertible_to_long_long()));
}

struct disabled_rvalue_conversion {
  operator const char*() const& { return "foo"; }
  operator const char*() & { return "foo"; }
  operator const char*() const&& = delete;
  operator const char*() && = delete;
};

TEST(core_test, disabled_rvalue_conversion) {
  EXPECT_EQ("foo", fmt::format("{}", disabled_rvalue_conversion()));
}

namespace adl_test {
template <typename... T> void make_format_args(const T&...) = delete;

struct string : std::string {};
}  // namespace adl_test

// Test that formatting functions compile when make_format_args is found by ADL.
TEST(core_test, adl) {
  // Only check compilation and don't run the code to avoid polluting the output
  // and since the output is tested elsewhere.
  if (fmt::detail::const_check(true)) return;
  auto s = adl_test::string();
  char buf[10];
  (void)fmt::format("{}", s);
  fmt::format_to(buf, "{}", s);
  fmt::format_to_n(buf, 10, "{}", s);
  (void)fmt::formatted_size("{}", s);
  fmt::print("{}", s);
  fmt::print(stdout, "{}", s);
}

TEST(core_test, has_const_formatter) {
  EXPECT_TRUE((fmt::detail::has_const_formatter<const_formattable,
                                                fmt::format_context>()));
  EXPECT_FALSE((fmt::detail::has_const_formatter<nonconst_formattable,
                                                 fmt::format_context>()));
}

TEST(core_test, format_nonconst) {
  EXPECT_EQ(fmt::format("{}", nonconst_formattable()), "test");
}
