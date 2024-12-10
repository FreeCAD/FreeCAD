// Formatting library for C++ - tests of the OS-specific functionality
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/os.h"

#include <cstdlib>  // std::exit
#include <cstring>
#include <memory>

#include "gtest-extra.h"
#include "util.h"

using fmt::buffered_file;
using testing::HasSubstr;
using wstring_view = fmt::basic_string_view<wchar_t>;

#ifdef _WIN32

#  include <windows.h>

TEST(util_test, utf16_to_utf8) {
  auto s = std::string("ёжик");
  fmt::detail::utf16_to_utf8 u(L"\x0451\x0436\x0438\x043A");
  EXPECT_EQ(s, u.str());
  EXPECT_EQ(s.size(), u.size());
}

TEST(util_test, utf16_to_utf8_empty_string) {
  std::string s = "";
  fmt::detail::utf16_to_utf8 u(L"");
  EXPECT_EQ(s, u.str());
  EXPECT_EQ(s.size(), u.size());
}

template <typename Converter, typename Char>
void check_utf_conversion_error(const char* message,
                                fmt::basic_string_view<Char> str =
                                    fmt::basic_string_view<Char>(nullptr, 1)) {
  fmt::memory_buffer out;
  fmt::detail::format_windows_error(out, ERROR_INVALID_PARAMETER, message);
  auto error = std::system_error(std::error_code());
  try {
    (Converter)(str);
  } catch (const std::system_error& e) {
    error = e;
  }
  EXPECT_EQ(ERROR_INVALID_PARAMETER, error.code().value());
  EXPECT_THAT(error.what(), HasSubstr(fmt::to_string(out)));
}

TEST(util_test, utf16_to_utf8_error) {
  check_utf_conversion_error<fmt::detail::utf16_to_utf8, wchar_t>(
      "cannot convert string from UTF-16 to UTF-8");
}

TEST(util_test, utf16_to_utf8_convert) {
  fmt::detail::utf16_to_utf8 u;
  EXPECT_EQ(ERROR_INVALID_PARAMETER, u.convert(wstring_view(nullptr, 1)));
  EXPECT_EQ(ERROR_INVALID_PARAMETER,
            u.convert(wstring_view(L"foo", INT_MAX + 1u)));
}

TEST(os_test, format_std_error_code) {
  EXPECT_EQ("generic:42",
            fmt::format(FMT_STRING("{0}"),
                        std::error_code(42, std::generic_category())));
  EXPECT_EQ("system:42",
            fmt::format(FMT_STRING("{0}"),
                        std::error_code(42, fmt::system_category())));
  EXPECT_EQ("system:-42",
            fmt::format(FMT_STRING("{0}"),
                        std::error_code(-42, fmt::system_category())));
}

TEST(os_test, format_windows_error) {
  LPWSTR message = nullptr;
  auto result = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, ERROR_FILE_EXISTS, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&message), 0, nullptr);
  fmt::detail::utf16_to_utf8 utf8_message(wstring_view(message, result - 2));
  LocalFree(message);
  fmt::memory_buffer actual_message;
  fmt::detail::format_windows_error(actual_message, ERROR_FILE_EXISTS, "test");
  EXPECT_EQ(fmt::format("test: {}", utf8_message.str()),
            fmt::to_string(actual_message));
  actual_message.resize(0);
}

TEST(os_test, format_long_windows_error) {
  LPWSTR message = nullptr;
  // this error code is not available on all Windows platforms and
  // Windows SDKs, so do not fail the test if the error string cannot
  // be retrieved.
  int provisioning_not_allowed = 0x80284013L;  // TBS_E_PROVISIONING_NOT_ALLOWED
  auto result = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, static_cast<DWORD>(provisioning_not_allowed),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&message), 0, nullptr);
  if (result == 0) {
    LocalFree(message);
    return;
  }
  fmt::detail::utf16_to_utf8 utf8_message(wstring_view(message, result - 2));
  LocalFree(message);
  fmt::memory_buffer actual_message;
  fmt::detail::format_windows_error(actual_message, provisioning_not_allowed,
                                    "test");
  EXPECT_EQ(fmt::format("test: {}", utf8_message.str()),
            fmt::to_string(actual_message));
}

TEST(os_test, windows_error) {
  auto error = std::system_error(std::error_code());
  try {
    throw fmt::windows_error(ERROR_FILE_EXISTS, "test {}", "error");
  } catch (const std::system_error& e) {
    error = e;
  }
  fmt::memory_buffer message;
  fmt::detail::format_windows_error(message, ERROR_FILE_EXISTS, "test error");
  EXPECT_THAT(error.what(), HasSubstr(to_string(message)));
  EXPECT_EQ(ERROR_FILE_EXISTS, error.code().value());
}

TEST(os_test, report_windows_error) {
  fmt::memory_buffer out;
  fmt::detail::format_windows_error(out, ERROR_FILE_EXISTS, "test error");
  out.push_back('\n');
  EXPECT_WRITE(stderr,
               fmt::report_windows_error(ERROR_FILE_EXISTS, "test error"),
               fmt::to_string(out));
}

#endif  // _WIN32

#if FMT_USE_FCNTL

using fmt::file;

bool isclosed(int fd) {
  char buffer;
  auto result = std::streamsize();
  SUPPRESS_ASSERT(result = FMT_POSIX(read(fd, &buffer, 1)));
  return result == -1 && errno == EBADF;
}

// Opens a file for reading.
file open_file() {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  write_end.write(file_content, std::strlen(file_content));
  write_end.close();
  return read_end;
}

// Attempts to write a string to a file.
void write(file& f, fmt::string_view s) {
  size_t num_chars_left = s.size();
  const char* ptr = s.data();
  do {
    size_t count = f.write(ptr, num_chars_left);
    ptr += count;
    // We can't write more than size_t bytes since num_chars_left
    // has type size_t.
    num_chars_left -= count;
  } while (num_chars_left != 0);
}

TEST(buffered_file_test, default_ctor) {
  auto f = buffered_file();
  EXPECT_TRUE(f.get() == nullptr);
}

TEST(buffered_file_test, move_ctor) {
  buffered_file bf = open_buffered_file();
  FILE* fp = bf.get();
  EXPECT_TRUE(fp != nullptr);
  buffered_file bf2(std::move(bf));
  EXPECT_EQ(fp, bf2.get());
  EXPECT_TRUE(bf.get() == nullptr);
}

TEST(buffered_file_test, move_assignment) {
  buffered_file bf = open_buffered_file();
  FILE* fp = bf.get();
  EXPECT_TRUE(fp != nullptr);
  buffered_file bf2;
  bf2 = std::move(bf);
  EXPECT_EQ(fp, bf2.get());
  EXPECT_TRUE(bf.get() == nullptr);
}

TEST(buffered_file_test, move_assignment_closes_file) {
  buffered_file bf = open_buffered_file();
  buffered_file bf2 = open_buffered_file();
  int old_fd = bf2.descriptor();
  bf2 = std::move(bf);
  EXPECT_TRUE(isclosed(old_fd));
}

TEST(buffered_file_test, move_from_temporary_in_ctor) {
  FILE* fp = nullptr;
  buffered_file f = open_buffered_file(&fp);
  EXPECT_EQ(fp, f.get());
}

TEST(buffered_file_test, move_from_temporary_in_assignment) {
  FILE* fp = nullptr;
  auto f = buffered_file();
  f = open_buffered_file(&fp);
  EXPECT_EQ(fp, f.get());
}

TEST(buffered_file_test, move_from_temporary_in_assignment_closes_file) {
  buffered_file f = open_buffered_file();
  int old_fd = f.descriptor();
  f = open_buffered_file();
  EXPECT_TRUE(isclosed(old_fd));
}

TEST(buffered_file_test, close_file_in_dtor) {
  int fd = 0;
  {
    buffered_file f = open_buffered_file();
    fd = f.descriptor();
  }
  EXPECT_TRUE(isclosed(fd));
}

TEST(buffered_file_test, close_error_in_dtor) {
  auto f =
      std::unique_ptr<buffered_file>(new buffered_file(open_buffered_file()));
  EXPECT_WRITE(
      stderr,
      {
        // The close function must be called inside EXPECT_WRITE,
        // otherwise the system may recycle closed file descriptor when
        // redirecting the output in EXPECT_STDERR and the second close
        // will break output redirection.
        FMT_POSIX(close(f->descriptor()));
        SUPPRESS_ASSERT(f.reset(nullptr));
      },
      system_error_message(EBADF, "cannot close file") + "\n");
}

TEST(buffered_file_test, close) {
  buffered_file f = open_buffered_file();
  int fd = f.descriptor();
  f.close();
  EXPECT_TRUE(f.get() == nullptr);
  EXPECT_TRUE(isclosed(fd));
}

TEST(buffered_file_test, close_error) {
  buffered_file f = open_buffered_file();
  FMT_POSIX(close(f.descriptor()));
  EXPECT_SYSTEM_ERROR_NOASSERT(f.close(), EBADF, "cannot close file");
  EXPECT_TRUE(f.get() == nullptr);
}

TEST(buffered_file_test, descriptor) {
  auto f = open_buffered_file();
  EXPECT_TRUE(f.descriptor() != -1);
  file copy = file::dup(f.descriptor());
  EXPECT_READ(copy, file_content);
}

TEST(ostream_test, move) {
  fmt::ostream out = fmt::output_file("test-file");
  fmt::ostream moved(std::move(out));
  moved.print("hello");
}

TEST(ostream_test, move_while_holding_data) {
  {
    fmt::ostream out = fmt::output_file("test-file");
    out.print("Hello, ");
    fmt::ostream moved(std::move(out));
    moved.print("world!\n");
  }
  {
    file in("test-file", file::RDONLY);
    EXPECT_READ(in, "Hello, world!\n");
  }
}

TEST(ostream_test, print) {
  fmt::ostream out = fmt::output_file("test-file");
  out.print("The answer is {}.\n",
            fmt::join(std::initializer_list<int>{42}, ", "));
  out.close();
  file in("test-file", file::RDONLY);
  EXPECT_READ(in, "The answer is 42.\n");
}

TEST(ostream_test, buffer_boundary) {
  auto str = std::string(4096, 'x');
  fmt::ostream out = fmt::output_file("test-file");
  out.print("{}", str);
  out.print("{}", str);
  out.close();
  file in("test-file", file::RDONLY);
  EXPECT_READ(in, str + str);
}

TEST(ostream_test, buffer_size) {
  fmt::ostream out = fmt::output_file("test-file", fmt::buffer_size = 1);
  out.print("{}", "foo");
  out.close();
  file in("test-file", file::RDONLY);
  EXPECT_READ(in, "foo");
}

TEST(ostream_test, truncate) {
  {
    fmt::ostream out = fmt::output_file("test-file");
    out.print("0123456789");
  }
  {
    fmt::ostream out = fmt::output_file("test-file");
    out.print("foo");
  }
  file in("test-file", file::RDONLY);
  EXPECT_EQ("foo", read(in, 4));
}

TEST(ostream_test, flush) {
  auto out = fmt::output_file("test-file");
  out.print("x");
  out.flush();
  auto in = fmt::file("test-file", file::RDONLY);
  EXPECT_READ(in, "x");
}

TEST(file_test, default_ctor) {
  file f;
  EXPECT_EQ(-1, f.descriptor());
}

TEST(file_test, open_buffered_file_in_ctor) {
  FILE* fp = safe_fopen("test-file", "w");
  std::fputs(file_content, fp);
  std::fclose(fp);
  file f("test-file", file::RDONLY);
  // Check if the file is open by reading one character from it.
  char buffer;
  bool isopen = FMT_POSIX(read(f.descriptor(), &buffer, 1)) == 1;
  ASSERT_TRUE(isopen);
}

TEST(file_test, open_buffered_file_error) {
  EXPECT_SYSTEM_ERROR(file("nonexistent", file::RDONLY), ENOENT,
                      "cannot open file nonexistent");
}

TEST(file_test, move_ctor) {
  file f = open_file();
  int fd = f.descriptor();
  EXPECT_NE(-1, fd);
  file f2(std::move(f));
  EXPECT_EQ(fd, f2.descriptor());
  EXPECT_EQ(-1, f.descriptor());
}

TEST(file_test, move_assignment) {
  file f = open_file();
  int fd = f.descriptor();
  EXPECT_NE(-1, fd);
  file f2;
  f2 = std::move(f);
  EXPECT_EQ(fd, f2.descriptor());
  EXPECT_EQ(-1, f.descriptor());
}

TEST(file_test, move_assignment_closes_file) {
  file f = open_file();
  file f2 = open_file();
  int old_fd = f2.descriptor();
  f2 = std::move(f);
  EXPECT_TRUE(isclosed(old_fd));
}

file open_buffered_file(int& fd) {
  file f = open_file();
  fd = f.descriptor();
  return f;
}

TEST(file_test, move_from_temporary_in_ctor) {
  int fd = 0xdead;
  file f(open_buffered_file(fd));
  EXPECT_EQ(fd, f.descriptor());
}

TEST(file_test, move_from_temporary_in_assignment) {
  int fd = 0xdead;
  file f;
  f = open_buffered_file(fd);
  EXPECT_EQ(fd, f.descriptor());
}

TEST(file_test, move_from_temporary_in_assignment_closes_file) {
  int fd = 0xdead;
  file f = open_file();
  int old_fd = f.descriptor();
  f = open_buffered_file(fd);
  EXPECT_TRUE(isclosed(old_fd));
}

TEST(file_test, close_file_in_dtor) {
  int fd = 0;
  {
    file f = open_file();
    fd = f.descriptor();
  }
  EXPECT_TRUE(isclosed(fd));
}

TEST(file_test, close_error_in_dtor) {
  std::unique_ptr<file> f(new file(open_file()));
  EXPECT_WRITE(
      stderr,
      {
        // The close function must be called inside EXPECT_WRITE,
        // otherwise the system may recycle closed file descriptor when
        // redirecting the output in EXPECT_STDERR and the second close
        // will break output redirection.
        FMT_POSIX(close(f->descriptor()));
        SUPPRESS_ASSERT(f.reset(nullptr));
      },
      system_error_message(EBADF, "cannot close file") + "\n");
}

TEST(file_test, close) {
  file f = open_file();
  int fd = f.descriptor();
  f.close();
  EXPECT_EQ(-1, f.descriptor());
  EXPECT_TRUE(isclosed(fd));
}

TEST(file_test, close_error) {
  file f = open_file();
  FMT_POSIX(close(f.descriptor()));
  EXPECT_SYSTEM_ERROR_NOASSERT(f.close(), EBADF, "cannot close file");
  EXPECT_EQ(-1, f.descriptor());
}

TEST(file_test, read) {
  file f = open_file();
  EXPECT_READ(f, file_content);
}

TEST(file_test, read_error) {
  file f("test-file", file::WRONLY);
  char buf;
  // We intentionally read from a file opened in the write-only mode to
  // cause error.
  EXPECT_SYSTEM_ERROR(f.read(&buf, 1), EBADF, "cannot read from file");
}

TEST(file_test, write) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  write(write_end, "test");
  write_end.close();
  EXPECT_READ(read_end, "test");
}

TEST(file_test, write_error) {
  file f("test-file", file::RDONLY);
  // We intentionally write to a file opened in the read-only mode to
  // cause error.
  EXPECT_SYSTEM_ERROR(f.write(" ", 1), EBADF, "cannot write to file");
}

TEST(file_test, dup) {
  file f = open_file();
  file copy = file::dup(f.descriptor());
  EXPECT_NE(f.descriptor(), copy.descriptor());
  EXPECT_EQ(file_content, read(copy, std::strlen(file_content)));
}

#  ifndef __COVERITY__
TEST(file_test, dup_error) {
  int value = -1;
  EXPECT_SYSTEM_ERROR_NOASSERT(file::dup(value), EBADF,
                               "cannot duplicate file descriptor -1");
}
#  endif

TEST(file_test, dup2) {
  file f = open_file();
  file copy = open_file();
  f.dup2(copy.descriptor());
  EXPECT_NE(f.descriptor(), copy.descriptor());
  EXPECT_READ(copy, file_content);
}

TEST(file_test, dup2_error) {
  file f = open_file();
  EXPECT_SYSTEM_ERROR_NOASSERT(
      f.dup2(-1), EBADF,
      fmt::format("cannot duplicate file descriptor {} to -1", f.descriptor()));
}

TEST(file_test, dup2_noexcept) {
  file f = open_file();
  file copy = open_file();
  std::error_code ec;
  f.dup2(copy.descriptor(), ec);
  EXPECT_EQ(ec.value(), 0);
  EXPECT_NE(f.descriptor(), copy.descriptor());
  EXPECT_READ(copy, file_content);
}

TEST(file_test, dup2_noexcept_error) {
  file f = open_file();
  std::error_code ec;
  SUPPRESS_ASSERT(f.dup2(-1, ec));
  EXPECT_EQ(EBADF, ec.value());
}

TEST(file_test, pipe) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  EXPECT_NE(-1, read_end.descriptor());
  EXPECT_NE(-1, write_end.descriptor());
  write(write_end, "test");
  EXPECT_READ(read_end, "test");
}

TEST(file_test, fdopen) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  int read_fd = read_end.descriptor();
  EXPECT_EQ(read_fd, FMT_POSIX(fileno(read_end.fdopen("r").get())));
}
#endif  // FMT_USE_FCNTL
