// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/xchar.h"

#include <complex>
#include <cwchar>
#include <vector>

#include "fmt/chrono.h"
#include "fmt/color.h"
#include "fmt/ostream.h"
#include "fmt/ranges.h"
#include "gtest-extra.h"  // Contains
#include "util.h"         // get_locale

using fmt::detail::max_value;
using testing::Contains;

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

struct non_string {};
}  // namespace test_ns

template <typename T> class is_string_test : public testing::Test {};

using string_char_types = testing::Types<char, wchar_t, char16_t, char32_t>;
TYPED_TEST_SUITE(is_string_test, string_char_types);

template <typename Char>
struct derived_from_string_view : fmt::basic_string_view<Char> {};

TYPED_TEST(is_string_test, is_string) {
  EXPECT_TRUE(fmt::detail::is_string<TypeParam*>::value);
  EXPECT_TRUE(fmt::detail::is_string<const TypeParam*>::value);
  EXPECT_TRUE(fmt::detail::is_string<TypeParam[2]>::value);
  EXPECT_TRUE(fmt::detail::is_string<const TypeParam[2]>::value);
  EXPECT_TRUE(fmt::detail::is_string<std::basic_string<TypeParam>>::value);
  EXPECT_TRUE(fmt::detail::is_string<fmt::basic_string_view<TypeParam>>::value);
  EXPECT_TRUE(
      fmt::detail::is_string<derived_from_string_view<TypeParam>>::value);
  using fmt_string_view = fmt::detail::std_string_view<TypeParam>;
  EXPECT_TRUE(std::is_empty<fmt_string_view>::value !=
              fmt::detail::is_string<fmt_string_view>::value);
  EXPECT_TRUE(fmt::detail::is_string<test_ns::test_string<TypeParam>>::value);
  EXPECT_FALSE(fmt::detail::is_string<test_ns::non_string>::value);
}

// std::is_constructible is broken in MSVC until version 2015.
#if !FMT_MSC_VERSION || FMT_MSC_VERSION >= 1900
struct explicitly_convertible_to_wstring_view {
  explicit operator fmt::wstring_view() const { return L"foo"; }
};

TEST(xchar_test, format_explicitly_convertible_to_wstring_view) {
  // Types explicitly convertible to wstring_view are not formattable by
  // default because it may introduce ODR violations.
  static_assert(
      !fmt::is_formattable<explicitly_convertible_to_wstring_view>::value, "");
}
#endif

TEST(xchar_test, format) {
  EXPECT_EQ(L"42", fmt::format(L"{}", 42));
  EXPECT_EQ(L"4.2", fmt::format(L"{}", 4.2));
  EXPECT_EQ(L"abc", fmt::format(L"{}", L"abc"));
  EXPECT_EQ(L"z", fmt::format(L"{}", L'z'));
  EXPECT_THROW(fmt::format(fmt::runtime(L"{:*\x343E}"), 42), fmt::format_error);
  EXPECT_EQ(L"true", fmt::format(L"{}", true));
  EXPECT_EQ(L"a", fmt::format(L"{0}", 'a'));
  EXPECT_EQ(L"a", fmt::format(L"{0}", L'a'));
  EXPECT_EQ(L"Cyrillic letter \x42e",
            fmt::format(L"Cyrillic letter {}", L'\x42e'));
  EXPECT_EQ(L"abc1", fmt::format(L"{}c{}", L"ab", 1));
}

TEST(xchar_test, is_formattable) {
  static_assert(!fmt::is_formattable<const wchar_t*>::value, "");
}

TEST(xchar_test, compile_time_string) {
  EXPECT_EQ(fmt::format(fmt::wformat_string<int>(L"{}"), 42), L"42");
#if defined(FMT_USE_STRING_VIEW) && FMT_CPLUSPLUS >= 201703L
  EXPECT_EQ(fmt::format(FMT_STRING(std::wstring_view(L"{}")), 42), L"42");
#endif
}

#if FMT_CPLUSPLUS > 201103L
struct custom_char {
  int value;
  custom_char() = default;

  template <typename T>
  constexpr custom_char(T val) : value(static_cast<int>(val)) {}

  operator int() const { return value; }
};

int to_ascii(custom_char c) { return c; }

FMT_BEGIN_NAMESPACE
template <> struct is_char<custom_char> : std::true_type {};
FMT_END_NAMESPACE

TEST(xchar_test, format_custom_char) {
  const custom_char format[] = {'{', '}', 0};
  auto result = fmt::format(format, custom_char('x'));
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], custom_char('x'));
}
#endif

// Convert a char8_t string to std::string. Otherwise GTest will insist on
// inserting `char8_t` NTBS into a `char` stream which is disabled by P1423.
template <typename S> std::string from_u8str(const S& str) {
  return std::string(str.begin(), str.end());
}

TEST(xchar_test, format_utf8_precision) {
  using str_type = std::basic_string<fmt::detail::char8_type>;
  auto format =
      str_type(reinterpret_cast<const fmt::detail::char8_type*>(u8"{:.4}"));
  auto str = str_type(reinterpret_cast<const fmt::detail::char8_type*>(
      u8"caf\u00e9s"));  // cafés
  auto result = fmt::format(format, str);
  EXPECT_EQ(fmt::detail::compute_width(result), 4);
  EXPECT_EQ(result.size(), 5);
  EXPECT_EQ(from_u8str(result), from_u8str(str.substr(0, 5)));
}

TEST(xchar_test, format_to) {
  auto buf = std::vector<wchar_t>();
  fmt::format_to(std::back_inserter(buf), L"{}{}", 42, L'\0');
  EXPECT_STREQ(buf.data(), L"42");
}

TEST(xchar_test, vformat_to) {
  using wcontext = fmt::wformat_context;
  fmt::basic_format_arg<wcontext> warg = fmt::detail::make_arg<wcontext>(42);
  auto wargs = fmt::basic_format_args<wcontext>(&warg, 1);
  auto w = std::wstring();
  fmt::vformat_to(std::back_inserter(w), L"{}", wargs);
  EXPECT_EQ(L"42", w);
  w.clear();
  fmt::vformat_to(std::back_inserter(w), FMT_STRING(L"{}"), wargs);
  EXPECT_EQ(L"42", w);
}

TEST(format_test, wide_format_to_n) {
  wchar_t buffer[4];
  buffer[3] = L'x';
  auto result = fmt::format_to_n(buffer, 3, L"{}", 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"123x", fmt::wstring_view(buffer, 4));
  buffer[0] = L'x';
  buffer[1] = L'x';
  buffer[2] = L'x';
  result = fmt::format_to_n(buffer, 3, L"{}", L'A');
  EXPECT_EQ(1u, result.size);
  EXPECT_EQ(buffer + 1, result.out);
  EXPECT_EQ(L"Axxx", fmt::wstring_view(buffer, 4));
  result = fmt::format_to_n(buffer, 3, L"{}{} ", L'B', L'C');
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"BC x", fmt::wstring_view(buffer, 4));
}

#if FMT_USE_USER_DEFINED_LITERALS
TEST(xchar_test, named_arg_udl) {
  using namespace fmt::literals;
  auto udl_a =
      fmt::format(L"{first}{second}{first}{third}", L"first"_a = L"abra",
                  L"second"_a = L"cad", L"third"_a = 99);
  EXPECT_EQ(
      fmt::format(L"{first}{second}{first}{third}", fmt::arg(L"first", L"abra"),
                  fmt::arg(L"second", L"cad"), fmt::arg(L"third", 99)),
      udl_a);
}
#endif  // FMT_USE_USER_DEFINED_LITERALS

TEST(xchar_test, print) {
  // Check that the wide print overload compiles.
  if (fmt::detail::const_check(false)) fmt::print(L"test");
}

TEST(xchar_test, join) {
  int v[3] = {1, 2, 3};
  EXPECT_EQ(fmt::format(L"({})", fmt::join(v, v + 3, L", ")), L"(1, 2, 3)");
  auto t = std::tuple<wchar_t, int, float>('a', 1, 2.0f);
  EXPECT_EQ(fmt::format(L"({})", fmt::join(t, L", ")), L"(a, 1, 2)");
}

enum streamable_enum {};

std::wostream& operator<<(std::wostream& os, streamable_enum) {
  return os << L"streamable_enum";
}

namespace fmt {
template <>
struct formatter<streamable_enum, wchar_t> : basic_ostream_formatter<wchar_t> {
};
}  // namespace fmt

enum unstreamable_enum {};
auto format_as(unstreamable_enum e) -> int { return e; }

TEST(xchar_test, enum) {
  EXPECT_EQ(L"streamable_enum", fmt::format(L"{}", streamable_enum()));
  EXPECT_EQ(L"0", fmt::format(L"{}", unstreamable_enum()));
}

struct streamable_and_unformattable {};

auto operator<<(std::wostream& os, streamable_and_unformattable)
    -> std::wostream& {
  return os << L"foo";
}

TEST(xchar_test, streamed) {
  EXPECT_FALSE(fmt::is_formattable<streamable_and_unformattable>());
  EXPECT_EQ(fmt::format(L"{}", fmt::streamed(streamable_and_unformattable())),
            L"foo");
}

TEST(xchar_test, sign_not_truncated) {
  wchar_t format_str[] = {
      L'{', L':',
      '+' | static_cast<wchar_t>(1 << fmt::detail::num_bits<char>()), L'}', 0};
  EXPECT_THROW(fmt::format(fmt::runtime(format_str), 42), fmt::format_error);
}

TEST(xchar_test, chrono) {
  auto tm = std::tm();
  tm.tm_year = 116;
  tm.tm_mon = 3;
  tm.tm_mday = 25;
  tm.tm_hour = 11;
  tm.tm_min = 22;
  tm.tm_sec = 33;
  EXPECT_EQ(fmt::format("The date is {:%Y-%m-%d %H:%M:%S}.", tm),
            "The date is 2016-04-25 11:22:33.");
  EXPECT_EQ(L"42s", fmt::format(L"{}", std::chrono::seconds(42)));
  EXPECT_EQ(fmt::format(L"{:%F}", tm), L"2016-04-25");
  EXPECT_EQ(fmt::format(L"{:%T}", tm), L"11:22:33");
}

std::wstring system_wcsftime(const std::wstring& format, const std::tm* timeptr,
                             std::locale* locptr = nullptr) {
  auto loc = locptr ? *locptr : std::locale::classic();
  auto& facet = std::use_facet<std::time_put<wchar_t>>(loc);
  std::wostringstream os;
  os.imbue(loc);
  facet.put(os, os, L' ', timeptr, format.c_str(),
            format.c_str() + format.size());
#ifdef _WIN32
  // Workaround a bug in older versions of Universal CRT.
  auto str = os.str();
  if (str == L"-0000") str = L"+0000";
  return str;
#else
  return os.str();
#endif
}

TEST(chrono_test_wchar, time_point) {
  auto t1 = std::chrono::system_clock::now();

  std::vector<std::wstring> spec_list = {
      L"%%",  L"%n",  L"%t",  L"%Y",  L"%EY", L"%y",  L"%Oy", L"%Ey", L"%C",
      L"%EC", L"%G",  L"%g",  L"%b",  L"%h",  L"%B",  L"%m",  L"%Om", L"%U",
      L"%OU", L"%W",  L"%OW", L"%V",  L"%OV", L"%j",  L"%d",  L"%Od", L"%e",
      L"%Oe", L"%a",  L"%A",  L"%w",  L"%Ow", L"%u",  L"%Ou", L"%H",  L"%OH",
      L"%I",  L"%OI", L"%M",  L"%OM", L"%S",  L"%OS", L"%x",  L"%Ex", L"%X",
      L"%EX", L"%D",  L"%F",  L"%R",  L"%T",  L"%p",  L"%z",  L"%Z"};
#ifndef _WIN32
  // Disabled on Windows, because these formats is not consistent among
  // platforms.
  spec_list.insert(spec_list.end(), {L"%c", L"%Ec", L"%r"});
#elif defined(__MINGW32__) && !defined(_UCRT)
  // Only C89 conversion specifiers when using MSVCRT instead of UCRT
  spec_list = {L"%%", L"%Y", L"%y", L"%b", L"%B", L"%m", L"%U",
               L"%W", L"%j", L"%d", L"%a", L"%A", L"%w", L"%H",
               L"%I", L"%M", L"%S", L"%x", L"%X", L"%p", L"%Z"};
#endif
  spec_list.push_back(L"%Y-%m-%d %H:%M:%S");

  for (const auto& spec : spec_list) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::localtime(&t);

    auto sys_output = system_wcsftime(spec, &tm);

    auto fmt_spec = fmt::format(L"{{:{}}}", spec);
    EXPECT_EQ(sys_output, fmt::format(fmt::runtime(fmt_spec), t1));
    EXPECT_EQ(sys_output, fmt::format(fmt::runtime(fmt_spec), tm));
  }
}

TEST(xchar_test, color) {
  EXPECT_EQ(fmt::format(fg(fmt::rgb(255, 20, 30)), L"rgb(255,20,30) wide"),
            L"\x1b[38;2;255;020;030mrgb(255,20,30) wide\x1b[0m");
}

TEST(xchar_test, ostream) {
#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 409
  std::wostringstream wos;
  fmt::print(wos, L"Don't {}!", L"panic");
  EXPECT_EQ(wos.str(), L"Don't panic!");
#endif
}

TEST(xchar_test, format_map) {
  auto m = std::map<std::wstring, int>{{L"one", 1}, {L"t\"wo", 2}};
  EXPECT_EQ(fmt::format(L"{}", m), L"{\"one\": 1, \"t\\\"wo\": 2}");
}

TEST(xchar_test, escape_string) {
  using vec = std::vector<std::wstring>;
  EXPECT_EQ(fmt::format(L"{}", vec{L"\n\r\t\"\\"}), L"[\"\\n\\r\\t\\\"\\\\\"]");
  EXPECT_EQ(fmt::format(L"{}", vec{L"понедельник"}), L"[\"понедельник\"]");
}

TEST(xchar_test, to_wstring) { EXPECT_EQ(L"42", fmt::to_wstring(42)); }

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
template <typename Char> struct numpunct : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const override { return '?'; }
  std::string do_grouping() const override { return "\03"; }
  Char do_thousands_sep() const override { return '~'; }
};

template <typename Char> struct no_grouping : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const override { return '.'; }
  std::string do_grouping() const override { return ""; }
  Char do_thousands_sep() const override { return ','; }
};

template <typename Char> struct special_grouping : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const override { return '.'; }
  std::string do_grouping() const override { return "\03\02"; }
  Char do_thousands_sep() const override { return ','; }
};

template <typename Char> struct small_grouping : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const override { return '.'; }
  std::string do_grouping() const override { return "\01"; }
  Char do_thousands_sep() const override { return ','; }
};

TEST(locale_test, localized_double) {
  auto loc = std::locale(std::locale(), new numpunct<char>());
  EXPECT_EQ(fmt::format(loc, "{:L}", 1.23), "1?23");
  EXPECT_EQ(fmt::format(loc, "{:Lf}", 1.23), "1?230000");
  EXPECT_EQ(fmt::format(loc, "{:L}", 1234.5), "1~234?5");
  EXPECT_EQ(fmt::format(loc, "{:L}", 12000.0), "12~000");
  EXPECT_EQ(fmt::format(loc, "{:8L}", 1230.0), "   1~230");
}

TEST(locale_test, format) {
  auto loc = std::locale(std::locale(), new numpunct<char>());
  EXPECT_EQ("1234567", fmt::format(std::locale(), "{:L}", 1234567));
  EXPECT_EQ("1~234~567", fmt::format(loc, "{:L}", 1234567));
  EXPECT_EQ("-1~234~567", fmt::format(loc, "{:L}", -1234567));
  EXPECT_EQ("-256", fmt::format(loc, "{:L}", -256));
  fmt::format_arg_store<fmt::format_context, int> as{1234567};
  EXPECT_EQ("1~234~567", fmt::vformat(loc, "{:L}", fmt::format_args(as)));
  auto s = std::string();
  fmt::format_to(std::back_inserter(s), loc, "{:L}", 1234567);
  EXPECT_EQ("1~234~567", s);

  auto no_grouping_loc = std::locale(std::locale(), new no_grouping<char>());
  EXPECT_EQ("1234567", fmt::format(no_grouping_loc, "{:L}", 1234567));

  auto special_grouping_loc =
      std::locale(std::locale(), new special_grouping<char>());
  EXPECT_EQ("1,23,45,678", fmt::format(special_grouping_loc, "{:L}", 12345678));
  EXPECT_EQ("12,345", fmt::format(special_grouping_loc, "{:L}", 12345));

  auto small_grouping_loc =
      std::locale(std::locale(), new small_grouping<char>());
  EXPECT_EQ("4,2,9,4,9,6,7,2,9,5",
            fmt::format(small_grouping_loc, "{:L}", max_value<uint32_t>()));
}

TEST(locale_test, format_detault_align) {
  auto loc = std::locale({}, new special_grouping<char>());
  EXPECT_EQ("  12,345", fmt::format(loc, "{:8L}", 12345));
}

TEST(locale_test, format_plus) {
  auto loc = std::locale({}, new special_grouping<char>());
  EXPECT_EQ("+100", fmt::format(loc, "{:+L}", 100));
}

TEST(locale_test, wformat) {
  auto loc = std::locale(std::locale(), new numpunct<wchar_t>());
  EXPECT_EQ(L"1234567", fmt::format(std::locale(), L"{:L}", 1234567));
  EXPECT_EQ(L"1~234~567", fmt::format(loc, L"{:L}", 1234567));
  using wcontext = fmt::buffer_context<wchar_t>;
  fmt::format_arg_store<wcontext, int> as{1234567};
  EXPECT_EQ(L"1~234~567",
            fmt::vformat(loc, L"{:L}", fmt::basic_format_args<wcontext>(as)));
  EXPECT_EQ(L"1234567", fmt::format(std::locale("C"), L"{:L}", 1234567));

  auto no_grouping_loc = std::locale(std::locale(), new no_grouping<wchar_t>());
  EXPECT_EQ(L"1234567", fmt::format(no_grouping_loc, L"{:L}", 1234567));

  auto special_grouping_loc =
      std::locale(std::locale(), new special_grouping<wchar_t>());
  EXPECT_EQ(L"1,23,45,678",
            fmt::format(special_grouping_loc, L"{:L}", 12345678));

  auto small_grouping_loc =
      std::locale(std::locale(), new small_grouping<wchar_t>());
  EXPECT_EQ(L"4,2,9,4,9,6,7,2,9,5",
            fmt::format(small_grouping_loc, L"{:L}", max_value<uint32_t>()));
}

TEST(locale_test, double_formatter) {
  auto loc = std::locale(std::locale(), new special_grouping<char>());
  auto f = fmt::formatter<int>();
  auto parse_ctx = fmt::format_parse_context("L");
  f.parse(parse_ctx);
  char buf[10] = {};
  fmt::basic_format_context<char*, char> format_ctx(
      buf, {}, fmt::detail::locale_ref(loc));
  *f.format(12345, format_ctx) = 0;
  EXPECT_STREQ("12,345", buf);
}

FMT_BEGIN_NAMESPACE
template <class charT> struct formatter<std::complex<double>, charT> {
 private:
  detail::dynamic_format_specs<char> specs_;

 public:
  FMT_CONSTEXPR typename basic_format_parse_context<charT>::iterator parse(
      basic_format_parse_context<charT>& ctx) {
    using handler_type =
        detail::dynamic_specs_handler<basic_format_parse_context<charT>>;
    detail::specs_checker<handler_type> handler(handler_type(specs_, ctx),
                                                detail::type::string_type);
    auto it = parse_format_specs(ctx.begin(), ctx.end(), handler);
    detail::parse_float_type_spec(specs_, ctx.error_handler());
    return it;
  }

  template <class FormatContext>
  typename FormatContext::iterator format(const std::complex<double>& c,
                                          FormatContext& ctx) {
    detail::handle_dynamic_spec<detail::precision_checker>(
        specs_.precision, specs_.precision_ref, ctx);
    auto specs = std::string();
    if (specs_.precision > 0) specs = fmt::format(".{}", specs_.precision);
    if (specs_.type == presentation_type::fixed_lower) specs += 'f';
    auto real = fmt::format(ctx.locale().template get<std::locale>(),
                            fmt::runtime("{:" + specs + "}"), c.real());
    auto imag = fmt::format(ctx.locale().template get<std::locale>(),
                            fmt::runtime("{:" + specs + "}"), c.imag());
    auto fill_align_width = std::string();
    if (specs_.width > 0) fill_align_width = fmt::format(">{}", specs_.width);
    return format_to(ctx.out(), runtime("{:" + fill_align_width + "}"),
                     c.real() != 0 ? fmt::format("({}+{}i)", real, imag)
                                   : fmt::format("{}i", imag));
  }
};
FMT_END_NAMESPACE

TEST(locale_test, complex) {
  std::string s = fmt::format("{}", std::complex<double>(1, 2));
  EXPECT_EQ(s, "(1+2i)");
  EXPECT_EQ(fmt::format("{:.2f}", std::complex<double>(1, 2)), "(1.00+2.00i)");
  EXPECT_EQ(fmt::format("{:8}", std::complex<double>(1, 2)), "  (1+2i)");
}

TEST(locale_test, chrono_weekday) {
  auto loc = get_locale("ru_RU.UTF-8", "Russian_Russia.1251");
  auto loc_old = std::locale::global(loc);
  auto mon = fmt::weekday(1);
  EXPECT_EQ(fmt::format(L"{}", mon), L"Mon");
  if (loc != std::locale::classic()) {
    // {L"\x43F\x43D", L"\x41F\x43D", L"\x43F\x43D\x434", L"\x41F\x43D\x434"}
    // {L"пн", L"Пн", L"пнд", L"Пнд"}
    EXPECT_THAT(
        (std::vector<std::wstring>{L"\x43F\x43D", L"\x41F\x43D",
                                   L"\x43F\x43D\x434", L"\x41F\x43D\x434"}),
        Contains(fmt::format(loc, L"{:L}", mon)));
  }
  std::locale::global(loc_old);
}

TEST(locale_test, sign) {
  EXPECT_EQ(fmt::format(std::locale(), L"{:L}", -50), L"-50");
}

#endif  // FMT_STATIC_THOUSANDS_SEPARATOR
