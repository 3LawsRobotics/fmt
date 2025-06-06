// Formatting library for C++ - tests of the OS-specific functionality
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.hpp.

#include "3laws/fmt/os.hpp"

#include <cstdlib>  // std::exit
#include <cstring>
#include <memory>

#include "gtest-extra.h"
#include "util.h"

using lll::fmt::buffered_file;
using testing::HasSubstr;
using wstring_view = lll::fmt::basic_string_view<wchar_t>;

#ifdef _WIN32

#  include <windows.h>

TEST(os_test, format_windows_error) {
  LPWSTR message = nullptr;
  auto result = FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, ERROR_FILE_EXISTS, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&message), 0, nullptr);
  auto utf8_message =
      lll::fmt::detail::to_utf8<wchar_t>(wstring_view(message, result - 2));
  LocalFree(message);
  lll::fmt::memory_buffer actual_message;
  lll::fmt::detail::format_windows_error(actual_message, ERROR_FILE_EXISTS,
                                         "test");
  EXPECT_EQ(lll::fmt::format("test: {}", utf8_message.str()),
            lll::fmt::to_string(actual_message));
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
  auto utf8_message =
      lll::fmt::detail::to_utf8<wchar_t>(wstring_view(message, result - 2));
  LocalFree(message);
  lll::fmt::memory_buffer actual_message;
  lll::fmt::detail::format_windows_error(actual_message,
                                         provisioning_not_allowed, "test");
  EXPECT_EQ(lll::fmt::format("test: {}", utf8_message.str()),
            lll::fmt::to_string(actual_message));
}

TEST(os_test, windows_error) {
  auto error = std::system_error(std::error_code());
  try {
    throw lll::fmt::windows_error(ERROR_FILE_EXISTS, "test {}", "error");
  } catch (const std::system_error& e) {
    error = e;
  }
  lll::fmt::memory_buffer message;
  lll::fmt::detail::format_windows_error(message, ERROR_FILE_EXISTS,
                                         "test error");
  EXPECT_THAT(error.what(), HasSubstr(to_string(message)));
  EXPECT_EQ(ERROR_FILE_EXISTS, error.code().value());
}

TEST(os_test, report_windows_error) {
  lll::fmt::memory_buffer out;
  lll::fmt::detail::format_windows_error(out, ERROR_FILE_EXISTS, "test error");
  out.push_back('\n');
  EXPECT_WRITE(stderr,
               lll::fmt::report_windows_error(ERROR_FILE_EXISTS, "test error"),
               lll::fmt::to_string(out));
}

#  if LAWS3_FMT_USE_FCNTL && !defined(__MINGW32__)
TEST(file_test, open_windows_file) {
  using lll::fmt::file;
  file out = file::open_windows_file(L"test-file",
                                     file::WRONLY | file::CREATE | file::TRUNC);
  out.write("x", 1);
  file in = file::open_windows_file(L"test-file", file::RDONLY);
  EXPECT_READ(in, "x");
}
#  endif  // LAWS3_FMT_USE_FCNTL && !defined(__MINGW32__)

#endif  // _WIN32

#if LAWS3_FMT_USE_FCNTL

using lll::fmt::file;

auto isclosed(int fd) -> bool {
  char buffer;
  auto result = std::streamsize();
  SUPPRESS_ASSERT(result = LAWS3_FMT_POSIX(read(fd, &buffer, 1)));
  return result == -1 && errno == EBADF;
}

// Opens a file for reading.
auto open_file() -> file {
  auto pipe = lll::fmt::pipe();
  pipe.write_end.write(file_content, std::strlen(file_content));
  pipe.write_end.close();
  return std::move(pipe.read_end);
}

// Attempts to write a string to a file.
void write(file& f, lll::fmt::string_view s) {
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
        LAWS3_FMT_POSIX(close(f->descriptor()));
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
  LAWS3_FMT_POSIX(close(f.descriptor()));
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
  lll::fmt::ostream out = lll::fmt::output_file("test-file");
  lll::fmt::ostream moved(std::move(out));
  moved.print("hello");
}

TEST(ostream_test, move_while_holding_data) {
  {
    lll::fmt::ostream out = lll::fmt::output_file("test-file");
    out.print("Hello, ");
    lll::fmt::ostream moved(std::move(out));
    moved.print("world!\n");
  }
  {
    file in("test-file", file::RDONLY);
    EXPECT_READ(in, "Hello, world!\n");
  }
}

TEST(ostream_test, print) {
  lll::fmt::ostream out = lll::fmt::output_file("test-file");
  out.print("The answer is {}.\n", 42);
  out.close();
  file in("test-file", file::RDONLY);
  EXPECT_READ(in, "The answer is 42.\n");
}

TEST(ostream_test, buffer_boundary) {
  auto str = std::string(4096, 'x');
  lll::fmt::ostream out = lll::fmt::output_file("test-file");
  out.print("{}", str);
  out.print("{}", str);
  out.close();
  file in("test-file", file::RDONLY);
  EXPECT_READ(in, str + str);
}

TEST(ostream_test, buffer_size) {
  lll::fmt::ostream out =
      lll::fmt::output_file("test-file", lll::fmt::buffer_size = 1);
  out.print("{}", "foo");
  out.close();
  file in("test-file", file::RDONLY);
  EXPECT_READ(in, "foo");
}

TEST(ostream_test, truncate) {
  {
    lll::fmt::ostream out = lll::fmt::output_file("test-file");
    out.print("0123456789");
  }
  {
    lll::fmt::ostream out = lll::fmt::output_file("test-file");
    out.print("foo");
  }
  file in("test-file", file::RDONLY);
  EXPECT_EQ("foo", read(in, 4));
}

TEST(ostream_test, flush) {
  auto out = lll::fmt::output_file("test-file");
  out.print("x");
  out.flush();
  auto in = lll::fmt::file("test-file", file::RDONLY);
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
  bool isopen = LAWS3_FMT_POSIX(read(f.descriptor(), &buffer, 1)) == 1;
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
        LAWS3_FMT_POSIX(close(f->descriptor()));
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
  LAWS3_FMT_POSIX(close(f.descriptor()));
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
  auto pipe = lll::fmt::pipe();
  write(pipe.write_end, "test");
  pipe.write_end.close();
  EXPECT_READ(pipe.read_end, "test");
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
      lll::fmt::format("cannot duplicate file descriptor {} to -1",
                       f.descriptor()));
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
  auto pipe = lll::fmt::pipe();
  EXPECT_NE(-1, pipe.read_end.descriptor());
  EXPECT_NE(-1, pipe.write_end.descriptor());
  write(pipe.write_end, "test");
  EXPECT_READ(pipe.read_end, "test");
}

TEST(file_test, fdopen) {
  auto pipe = lll::fmt::pipe();
  int read_fd = pipe.read_end.descriptor();
  EXPECT_EQ(read_fd, LAWS3_FMT_POSIX(fileno(pipe.read_end.fdopen("r").get())));
}
#endif  // LAWS3_FMT_USE_FCNTL
