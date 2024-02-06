// Formatting library for C++ - module tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.hpp.
//
// Copyright (c) 2021 - present, Daniela Engert
// All Rights Reserved
// {fmt} module.

#ifdef _MSC_FULL_VER
// hide some implementation bugs in msvc
// that are not essential to users of the module.
#  define LAWS3_FMT_HIDE_MODULE_BUGS
#endif

#include <bit>
#include <chrono>
#include <exception>
#include <iterator>
#include <locale>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>

#if (__has_include(<fcntl.h>) || defined(__APPLE__) || \
     defined(__linux__)) &&                              \
    (!defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP))
#  include <fcntl.h>
#  define LAWS3_FMT_USE_FCNTL 1
#else
#  define LAWS3_FMT_USE_FCNTL 0
#endif
#if defined(_WIN32) && !defined(__MINGW32__)
#  define LAWS3_FMT_POSIX(call) _##call
#else
#  define LAWS3_FMT_POSIX(call) call
#endif

import fmt;

// check for macros leaking from BMI
static bool macro_leaked =
#if defined(LAWS3_FMT_CORE_H_) || defined(LAWS3_FMT_FORMAT_H_)
    true;
#else
    false;
#endif

#define LAWS3_FMT_OS_H_  // don't pull in os.hpp, neither directly nor
                         // indirectly
#include "gtest-extra.h"

// an implicitly exported namespace must be visible [module.interface]/2.2
TEST(module_test, namespace) {
  using namespace fmt;
  using namespace lll::fmt::literals;
  ASSERT_TRUE(true);
}

namespace detail {
bool oops_detail_namespace_is_visible;
}

namespace lll {
namespace fmt {
bool namespace_detail_invisible() {
#if defined(LAWS3_FMT_HIDE_MODULE_BUGS) && defined(_MSC_FULL_VER) && \
    _MSC_FULL_VER <= 193700000
  // bug in msvc up to at least 17.7:

  // the namespace is visible even when it is neither
  // implicitly nor explicitly exported
  return true;
#else
  using namespace detail;
  // this fails to compile if lll::fmt::detail is visible
  return !oops_detail_namespace_is_visible;
#endif
}
}  // namespace fmt
}  // namespace lll

// the non-exported namespace 'detail' must be invisible [module.interface]/2
TEST(module_test, detail_namespace) {
  EXPECT_TRUE(lll::fmt::namespace_detail_invisible());
}

// macros must not be imported from a *named* module  [cpp.import]/5.1
TEST(module_test, macros) {
#if defined(LAWS3_FMT_HIDE_MODULE_BUGS) && defined(_MSC_FULL_VER) && \
    _MSC_FULL_VER <= 192930130
  // bug in msvc up to 16.11-pre2:
  // include-guard macros leak from BMI
  // and even worse: they cannot be #undef-ined
  macro_leaked = false;
#endif
  EXPECT_FALSE(macro_leaked);
}

// The following is less about functional testing (that's done elsewhere)
// but rather visibility of all client-facing overloads, reachability of
// non-exported entities, name lookup and overload resolution within
// template instantitions.
// Exercise all exported entities of the API at least once.
// Instantiate as many code paths as possible.

TEST(module_test, to_string) {
  EXPECT_EQ("42", lll::fmt::to_string(42));
  EXPECT_EQ("42", lll::fmt::to_string(42.0));

  EXPECT_EQ(L"42", lll::fmt::to_wstring(42));
  EXPECT_EQ(L"42", lll::fmt::to_wstring(42.0));
}

TEST(module_test, format) {
  EXPECT_EQ("42", lll::fmt::format("{:}", 42));
  EXPECT_EQ("-42", lll::fmt::format("{0}", -42.0));

  EXPECT_EQ(L"42", lll::fmt::format(L"{:}", 42));
  EXPECT_EQ(L"-42", lll::fmt::format(L"{0}", -42.0));
}

TEST(module_test, format_to) {
  std::string s;
  lll::fmt::format_to(std::back_inserter(s), "{}", 42);
  EXPECT_EQ("42", s);

  char buffer[4] = {0};
  lll::fmt::format_to(buffer, "{}", 42);
  EXPECT_EQ("42", std::string_view(buffer));

  lll::fmt::memory_buffer mb;
  lll::fmt::format_to(std::back_inserter(mb), "{}", 42);
  EXPECT_EQ("42", std::string_view(buffer));

  std::wstring w;
  lll::fmt::format_to(std::back_inserter(w), L"{}", 42);
  EXPECT_EQ(L"42", w);

  wchar_t wbuffer[4] = {0};
  lll::fmt::format_to(wbuffer, L"{}", 42);
  EXPECT_EQ(L"42", std::wstring_view(wbuffer));

  lll::fmt::wmemory_buffer wb;
  lll::fmt::format_to(std::back_inserter(wb), L"{}", 42);
  EXPECT_EQ(L"42", std::wstring_view(wbuffer));
}

TEST(module_test, formatted_size) {
  EXPECT_EQ(2u, lll::fmt::formatted_size("{}", 42));
  EXPECT_EQ(2u, lll::fmt::formatted_size(L"{}", 42));
}

TEST(module_test, format_to_n) {
  std::string s;
  auto result = lll::fmt::format_to_n(std::back_inserter(s), 1, "{}", 42);
  EXPECT_EQ(2u, result.size);
  char buffer[4] = {0};
  lll::fmt::format_to_n(buffer, 3, "{}", 12345);

  std::wstring w;
  auto wresult = lll::fmt::format_to_n(std::back_inserter(w), 1, L"{}", 42);
  EXPECT_EQ(2u, wresult.size);
  wchar_t wbuffer[4] = {0};
  lll::fmt::format_to_n(wbuffer, 3, L"{}", 12345);
}

TEST(module_test, format_args) {
  auto no_args = lll::fmt::format_args();
  EXPECT_FALSE(no_args.get(1));

  lll::fmt::basic_format_args args = lll::fmt::make_format_args(42);
  EXPECT_TRUE(args.max_size() > 0);
  auto arg0 = args.get(0);
  EXPECT_TRUE(arg0);
  decltype(arg0) arg_none;
  EXPECT_FALSE(arg_none);
  EXPECT_TRUE(arg0.type() != arg_none.type());
}

TEST(module_test, wformat_args) {
  auto no_args = lll::fmt::wformat_args();
  EXPECT_FALSE(no_args.get(1));
  lll::fmt::basic_format_args args = lll::fmt::make_wformat_args(42);
  EXPECT_TRUE(args.get(0));
}

TEST(module_test, dynamic_format_args) {
  lll::fmt::dynamic_format_arg_store<lll::fmt::format_context> dyn_store;
  dyn_store.push_back(lll::fmt::arg("a42", 42));
  lll::fmt::basic_format_args args = dyn_store;
  EXPECT_FALSE(args.get(3));
  EXPECT_TRUE(args.get(lll::fmt::string_view("a42")));

  lll::fmt::dynamic_format_arg_store<lll::fmt::wformat_context> wdyn_store;
  wdyn_store.push_back(lll::fmt::arg(L"a42", 42));
  lll::fmt::basic_format_args wargs = wdyn_store;
  EXPECT_FALSE(wargs.get(3));
  EXPECT_TRUE(wargs.get(lll::fmt::wstring_view(L"a42")));
}

TEST(module_test, vformat) {
  EXPECT_EQ("42", lll::fmt::vformat("{}", lll::fmt::make_format_args(42)));
  EXPECT_EQ(L"42", lll::fmt::vformat(lll::fmt::wstring_view(L"{}"),
                                     lll::fmt::make_wformat_args(42)));
}

TEST(module_test, vformat_to) {
  auto store = lll::fmt::make_format_args(42);
  std::string s;
  lll::fmt::vformat_to(std::back_inserter(s), "{}", store);
  EXPECT_EQ("42", s);

  char buffer[4] = {0};
  lll::fmt::vformat_to(buffer, "{:}", store);
  EXPECT_EQ("42", std::string_view(buffer));

  auto wstore = lll::fmt::make_wformat_args(42);
  std::wstring w;
  lll::fmt::vformat_to(std::back_inserter(w), L"{}", wstore);
  EXPECT_EQ(L"42", w);

  wchar_t wbuffer[4] = {0};
  lll::fmt::vformat_to(wbuffer, L"{:}", wstore);
  EXPECT_EQ(L"42", std::wstring_view(wbuffer));
}

TEST(module_test, vformat_to_n) {
  auto store = lll::fmt::make_format_args(12345);
  std::string s;
  auto result = lll::fmt::vformat_to_n(std::back_inserter(s), 1, "{}", store);
  char buffer[4] = {0};
  lll::fmt::vformat_to_n(buffer, 3, "{:}", store);

  auto wstore = lll::fmt::make_wformat_args(12345);
  std::wstring w;
  auto wresult = lll::fmt::vformat_to_n(std::back_inserter(w), 1,
                                        lll::fmt::wstring_view(L"{}"), wstore);
  wchar_t wbuffer[4] = {0};
  lll::fmt::vformat_to_n(wbuffer, 3, lll::fmt::wstring_view(L"{:}"), wstore);
}

std::string as_string(std::wstring_view text) {
  return {reinterpret_cast<const char*>(text.data()),
          text.size() * sizeof(text[0])};
}

TEST(module_test, print) {
  EXPECT_WRITE(stdout, lll::fmt::print("{}µ", 42), "42µ");
  EXPECT_WRITE(stderr, lll::fmt::print(stderr, "{}µ", 4.2), "4.2µ");
  EXPECT_WRITE(stdout, lll::fmt::print(L"{}µ", 42), as_string(L"42µ"));
  EXPECT_WRITE(stderr, lll::fmt::print(stderr, L"{}µ", 4.2),
               as_string(L"4.2µ"));
}

TEST(module_test, vprint) {
  EXPECT_WRITE(stdout, lll::fmt::vprint("{:}µ", lll::fmt::make_format_args(42)),
               "42µ");
  EXPECT_WRITE(stderr,
               lll::fmt::vprint(stderr, "{}", lll::fmt::make_format_args(4.2)),
               "4.2");
  EXPECT_WRITE(stdout,
               lll::fmt::vprint(L"{:}µ", lll::fmt::make_wformat_args(42)),
               as_string(L"42µ"));
  EXPECT_WRITE(stderr,
               lll::fmt::vprint(stderr, L"{}", lll::fmt::make_wformat_args(42)),
               as_string(L"42"));
}

TEST(module_test, named_args) {
  EXPECT_EQ("42", lll::fmt::format("{answer}", lll::fmt::arg("answer", 42)));
  EXPECT_EQ(L"42", lll::fmt::format(L"{answer}", lll::fmt::arg(L"answer", 42)));
}

TEST(module_test, literals) {
  using namespace lll::fmt::literals;
  EXPECT_EQ("42", lll::fmt::format("{answer}", "answer"_a = 42));
  EXPECT_EQ(L"42", lll::fmt::format(L"{answer}", L"answer"_a = 42));
}

TEST(module_test, locale) {
  auto store = lll::fmt::make_format_args(4.2);
  const auto classic = std::locale::classic();
  EXPECT_EQ("4.2", lll::fmt::format(classic, "{:L}", 4.2));
  EXPECT_EQ("4.2", lll::fmt::vformat(classic, "{:L}", store));
  std::string s;
  lll::fmt::vformat_to(std::back_inserter(s), classic, "{:L}", store);
  EXPECT_EQ("4.2", s);
  EXPECT_EQ("4.2", lll::fmt::format("{:L}", 4.2));

  auto wstore = lll::fmt::make_wformat_args(4.2);
  EXPECT_EQ(L"4.2", lll::fmt::format(classic, L"{:L}", 4.2));
  EXPECT_EQ(L"4.2", lll::fmt::vformat(classic, L"{:L}", wstore));
  std::wstring w;
  lll::fmt::vformat_to(std::back_inserter(w), classic, L"{:L}", wstore);
  EXPECT_EQ(L"4.2", w);
  EXPECT_EQ(L"4.2", lll::fmt::format(L"{:L}", 4.2));
}

TEST(module_test, string_view) {
  lll::fmt::string_view nsv("fmt");
  EXPECT_EQ("fmt", nsv);
  EXPECT_TRUE(lll::fmt::string_view("fmt") == nsv);

  lll::fmt::wstring_view wsv(L"fmt");
  EXPECT_EQ(L"fmt", wsv);
  EXPECT_TRUE(lll::fmt::wstring_view(L"fmt") == wsv);
}

TEST(module_test, memory_buffer) {
  lll::fmt::basic_memory_buffer<char, lll::fmt::inline_buffer_size> buffer;
  lll::fmt::format_to(std::back_inserter(buffer), "{}", "42");
  EXPECT_EQ("42", to_string(buffer));
  lll::fmt::memory_buffer nbuffer(std::move(buffer));
  EXPECT_EQ("42", to_string(nbuffer));
  buffer = std::move(nbuffer);
  EXPECT_EQ("42", to_string(buffer));
  nbuffer.clear();
  EXPECT_EQ(0u, to_string(nbuffer).size());

  lll::fmt::wmemory_buffer wbuffer;
  EXPECT_EQ(0u, to_string(wbuffer).size());
}

TEST(module_test, is_char) {
  EXPECT_TRUE(lll::fmt::is_char<char>());
  EXPECT_TRUE(lll::fmt::is_char<wchar_t>());
  EXPECT_TRUE(lll::fmt::is_char<char8_t>());
  EXPECT_TRUE(lll::fmt::is_char<char16_t>());
  EXPECT_TRUE(lll::fmt::is_char<char32_t>());
  EXPECT_FALSE(lll::fmt::is_char<signed char>());
}

TEST(module_test, ptr) {
  uintptr_t answer = 42;
  auto p = std::bit_cast<int*>(answer);
  EXPECT_EQ("0x2a", lll::fmt::to_string(lll::fmt::ptr(p)));
  std::unique_ptr<int> up(p);
  EXPECT_EQ("0x2a", lll::fmt::to_string(lll::fmt::ptr(up)));
  up.release();
  auto sp = std::make_shared<int>(0);
  p = sp.get();
  EXPECT_EQ(lll::fmt::to_string(lll::fmt::ptr(p)),
            lll::fmt::to_string(lll::fmt::ptr(sp)));
}

TEST(module_test, errors) {
  auto store = lll::fmt::make_format_args(42);
  EXPECT_THROW(throw lll::fmt::format_error("oops"), std::exception);
  EXPECT_THROW(throw lll::fmt::vsystem_error(0, "{}", store),
               std::system_error);
  EXPECT_THROW(throw lll::fmt::system_error(0, "{}", 42), std::system_error);

  lll::fmt::memory_buffer buffer;
  lll::fmt::format_system_error(buffer, 0, "oops");
  auto oops = to_string(buffer);
  EXPECT_TRUE(oops.size() > 0);
  EXPECT_WRITE(stderr, lll::fmt::report_system_error(0, "oops"), oops + '\n');

#ifdef _WIN32
  EXPECT_THROW(throw lll::fmt::vwindows_error(0, "{}", store),
               std::system_error);
  EXPECT_THROW(throw lll::fmt::windows_error(0, "{}", 42), std::system_error);
  output_redirect redirect(stderr);
  lll::fmt::report_windows_error(0, "oops");
  EXPECT_TRUE(redirect.restore_and_read().size() > 0);
#endif
}

TEST(module_test, error_code) {
  EXPECT_EQ(
      "generic:42",
      lll::fmt::format("{0}", std::error_code(42, std::generic_category())));
  EXPECT_EQ("system:42",
            lll::fmt::format("{0}",
                             std::error_code(42, lll::fmt::system_category())));
  EXPECT_EQ(
      L"generic:42",
      lll::fmt::format(L"{0}", std::error_code(42, std::generic_category())));
}

TEST(module_test, format_int) {
  lll::fmt::format_int sanswer(42);
  EXPECT_EQ("42", lll::fmt::string_view(sanswer.data(), sanswer.size()));
  lll::fmt::format_int uanswer(42u);
  EXPECT_EQ("42", lll::fmt::string_view(uanswer.data(), uanswer.size()));
}

struct test_formatter : lll::fmt::formatter<char> {
  bool check() { return true; }
};

TEST(module_test, formatter) { EXPECT_TRUE(test_formatter{}.check()); }

TEST(module_test, join) {
  int arr[3] = {1, 2, 3};
  std::vector<double> vec{1.0, 2.0, 3.0};
  std::initializer_list<int> il{1, 2, 3};
  auto sep = lll::fmt::string_view(", ");
  EXPECT_EQ("1, 2, 3", to_string(lll::fmt::join(arr + 0, arr + 3, sep)));
  EXPECT_EQ("1, 2, 3", to_string(lll::fmt::join(arr, sep)));
  EXPECT_EQ("1, 2, 3", to_string(lll::fmt::join(vec.begin(), vec.end(), sep)));
  EXPECT_EQ("1, 2, 3", to_string(lll::fmt::join(vec, sep)));
  EXPECT_EQ("1, 2, 3", to_string(lll::fmt::join(il, sep)));

  auto wsep = lll::fmt::wstring_view(L", ");
  EXPECT_EQ(L"1, 2, 3",
            lll::fmt::format(L"{}", lll::fmt::join(arr + 0, arr + 3, wsep)));
  EXPECT_EQ(L"1, 2, 3", lll::fmt::format(L"{}", lll::fmt::join(arr, wsep)));
  EXPECT_EQ(L"1, 2, 3", lll::fmt::format(L"{}", lll::fmt::join(il, wsep)));
}

TEST(module_test, time) {
  auto time_now = std::time(nullptr);
  EXPECT_TRUE(lll::fmt::localtime(time_now).tm_year > 120);
  EXPECT_TRUE(lll::fmt::gmtime(time_now).tm_year > 120);
  auto chrono_now = std::chrono::system_clock::now();
  EXPECT_TRUE(lll::fmt::gmtime(chrono_now).tm_year > 120);
}

TEST(module_test, time_point) {
  auto now = std::chrono::system_clock::now();
  std::string_view past("2021-05-20 10:30:15");
  EXPECT_TRUE(past < lll::fmt::format("{:%Y-%m-%d %H:%M:%S}", now));
  std::wstring_view wpast(L"2021-05-20 10:30:15");
  EXPECT_TRUE(wpast < lll::fmt::format(L"{:%Y-%m-%d %H:%M:%S}", now));
}

TEST(module_test, time_duration) {
  using us = std::chrono::duration<double, std::micro>;
  EXPECT_EQ("42s", lll::fmt::format("{}", std::chrono::seconds{42}));
  EXPECT_EQ("4.2µs", lll::fmt::format("{:3.1}", us{4.234}));
  EXPECT_EQ("4.2µs", lll::fmt::format(std::locale::classic(), "{:L}", us{4.2}));

  EXPECT_EQ(L"42s", lll::fmt::format(L"{}", std::chrono::seconds{42}));
  EXPECT_EQ(L"4.2µs", lll::fmt::format(L"{:3.1}", us{4.234}));
  EXPECT_EQ(L"4.2µs",
            lll::fmt::format(std::locale::classic(), L"{:L}", us{4.2}));
}

TEST(module_test, weekday) {
  EXPECT_EQ("Mon", lll::fmt::format(std::locale::classic(), "{}",
                                    lll::fmt::weekday(1)));
}

TEST(module_test, printf) {
  EXPECT_WRITE(stdout, lll::fmt::printf("%f", 42.123456), "42.123456");
  EXPECT_WRITE(stdout, lll::fmt::printf("%d", 42), "42");
  EXPECT_WRITE(stdout, lll::fmt::printf(L"%f", 42.123456),
               as_string(L"42.123456"));
  EXPECT_WRITE(stdout, lll::fmt::printf(L"%d", 42), as_string(L"42"));
}

TEST(module_test, fprintf) {
  EXPECT_WRITE(stderr, lll::fmt::fprintf(stderr, "%d", 42), "42");
  EXPECT_WRITE(stderr, lll::fmt::fprintf(stderr, L"%d", 42), as_string(L"42"));
}

TEST(module_test, sprintf) {
  EXPECT_EQ("42", lll::fmt::sprintf("%d", 42));
  EXPECT_EQ(L"42", lll::fmt::sprintf(L"%d", 42));
}

TEST(module_test, vprintf) {
  EXPECT_WRITE(stdout, lll::fmt::vprintf("%d", lll::fmt::make_printf_args(42)),
               "42");
  EXPECT_WRITE(stdout,
               lll::fmt::vprintf(L"%d", lll::fmt::make_wprintf_args(42)),
               as_string(L"42"));
}

TEST(module_test, vfprintf) {
  auto args = lll::fmt::make_printf_args(42);
  EXPECT_WRITE(stderr, lll::fmt::vfprintf(stderr, "%d", args), "42");
  auto wargs = lll::fmt::make_wprintf_args(42);
  EXPECT_WRITE(stderr, lll::fmt::vfprintf(stderr, L"%d", wargs),
               as_string(L"42"));
}

TEST(module_test, vsprintf) {
  EXPECT_EQ("42", lll::fmt::vsprintf("%d", lll::fmt::make_printf_args(42)));
  EXPECT_EQ(L"42", lll::fmt::vsprintf(L"%d", lll::fmt::make_wprintf_args(42)));
}

TEST(module_test, color) {
  auto fg_check = fg(lll::fmt::rgb(255, 200, 30));
  auto bg_check =
      bg(lll::fmt::color::dark_slate_gray) | lll::fmt::emphasis::italic;
  auto emphasis_check =
      lll::fmt::emphasis::underline | lll::fmt::emphasis::bold;
  EXPECT_EQ("\x1B[30m42\x1B[0m",
            lll::fmt::format(fg(lll::fmt::terminal_color::black), "{}", 42));
  EXPECT_EQ(L"\x1B[30m42\x1B[0m",
            lll::fmt::format(fg(lll::fmt::terminal_color::black), L"{}", 42));
}

TEST(module_test, cstring_view) {
  auto s = "fmt";
  EXPECT_EQ(s, lll::fmt::cstring_view(s).c_str());
  auto w = L"fmt";
  EXPECT_EQ(w, lll::fmt::wcstring_view(w).c_str());
}

TEST(module_test, buffered_file) {
  EXPECT_TRUE(lll::fmt::buffered_file{}.get() == nullptr);
}

TEST(module_test, output_file) {
#ifdef __clang__
  lll::fmt::println("\033[0;33m[=disabled=] {}\033[0;0m",
                    "Clang 16.0 emits multiple copies of vtables");
#else
  lll::fmt::ostream out =
      lll::fmt::output_file("module-test", lll::fmt::buffer_size = 1);
  out.close();
#endif
}

struct custom_context {
  using char_type = char;
  using parse_context_type = lll::fmt::format_parse_context;
};

TEST(module_test, custom_context) {
  lll::fmt::basic_format_arg<custom_context> custom_arg;
  EXPECT_TRUE(!custom_arg);
}

TEST(module_test, compile_format_string) {
  using namespace lll::fmt::literals;
#ifdef __clang__
  lll::fmt::println("\033[0;33m[=disabled=] {}\033[0;0m",
                    "Clang 16.0 fails to import user-defined literals");
#else
  EXPECT_EQ("42", lll::fmt::format("{0:x}"_cf, 0x42));
  EXPECT_EQ(L"42", lll::fmt::format(L"{:}"_cf, 42));
  EXPECT_EQ("4.2", lll::fmt::format("{arg:3.1f}"_cf, "arg"_a = 4.2));
  EXPECT_EQ(L" 42", lll::fmt::format(L"{arg:>3}"_cf, L"arg"_a = L"42"));
#endif
}
