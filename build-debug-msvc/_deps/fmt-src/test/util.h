// Formatting library for C++ - test utilities
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <cstdarg>
#include <cstdio>
#include <locale>
#include <string>

#ifdef FMT_MODULE_TEST
import fmt;
#else
#  include "fmt/os.h"
#endif  // FMT_MODULE_TEST

#ifdef _MSC_VER
#  define FMT_VSNPRINTF vsprintf_s
#else
#  define FMT_VSNPRINTF vsnprintf
#endif

template <size_t SIZE>
void safe_sprintf(char (&buffer)[SIZE], const char* format, ...) {
  std::va_list args;
  va_start(args, format);
  FMT_VSNPRINTF(buffer, SIZE, format, args);
  va_end(args);
}

extern const char* const file_content;

// Opens a buffered file for reading.
fmt::buffered_file open_buffered_file(FILE** fp = nullptr);

inline FILE* safe_fopen(const char* filename, const char* mode) {
#if defined(_WIN32) && !defined(__MINGW32__)
  // Fix MSVC warning about "unsafe" fopen.
  FILE* f = nullptr;
  errno = fopen_s(&f, filename, mode);
  return f;
#else
  return std::fopen(filename, mode);
#endif
}

template <typename Char> class basic_test_string {
 private:
  std::basic_string<Char> value_;

  static const Char empty[];

 public:
  explicit basic_test_string(const Char* value = empty) : value_(value) {}

  const std::basic_string<Char>& value() const { return value_; }
};

template <typename Char> const Char basic_test_string<Char>::empty[] = {0};

typedef basic_test_string<char> test_string;
typedef basic_test_string<wchar_t> test_wstring;

template <typename Char>
std::basic_ostream<Char>& operator<<(std::basic_ostream<Char>& os,
                                     const basic_test_string<Char>& s) {
  os << s.value();
  return os;
}

class date {
  int year_, month_, day_;

 public:
  date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

  int year() const { return year_; }
  int month() const { return month_; }
  int day() const { return day_; }
};

// Returns a locale with the given name if available or classic locale othewise.
std::locale get_locale(const char* name, const char* alt_name = nullptr);
