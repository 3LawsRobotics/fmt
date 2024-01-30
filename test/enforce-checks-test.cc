// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <iterator>
#include <vector>

#define I 42  // simulate https://en.cppreference.com/w/c/numeric/complex/I
#include "fmt/chrono.h"
#include "fmt/color.h"
#include "fmt/format.h"
#include "fmt/ostream.h"
#include "fmt/ranges.h"
#include "fmt/xchar.h"
#undef I

// Exercise the API to verify that everything we expect to can compile.
void test_format_api() {
  (void)lll::fmt::format(LAWS3_FMT_STRING("{}"), 42);
  (void)lll::fmt::format(LAWS3_FMT_STRING(L"{}"), 42);
  (void)lll::fmt::format(LAWS3_FMT_STRING("noop"));

  (void)lll::fmt::to_string(42);
  (void)lll::fmt::to_wstring(42);

  std::vector<char> out;
  lll::fmt::format_to(std::back_inserter(out), LAWS3_FMT_STRING("{}"), 42);

  char buffer[4];
  lll::fmt::format_to_n(buffer, 3, LAWS3_FMT_STRING("{}"), 12345);

  wchar_t wbuffer[4];
  lll::fmt::format_to_n(wbuffer, 3, LAWS3_FMT_STRING(L"{}"), 12345);
}

void test_chrono() {
  (void)lll::fmt::format(LAWS3_FMT_STRING("{}"), std::chrono::seconds(42));
  (void)lll::fmt::format(LAWS3_FMT_STRING(L"{}"), std::chrono::seconds(42));
}

void test_text_style() {
  lll::fmt::print(fg(lll::fmt::rgb(255, 20, 30)), LAWS3_FMT_STRING("{}"),
                  "rgb(255,20,30)");
  (void)lll::fmt::format(fg(lll::fmt::rgb(255, 20, 30)), LAWS3_FMT_STRING("{}"),
                         "rgb(255,20,30)");

  lll::fmt::text_style ts = fg(lll::fmt::rgb(255, 20, 30));
  std::string out;
  lll::fmt::format_to(std::back_inserter(out), ts,
                      LAWS3_FMT_STRING("rgb(255,20,30){}{}{}"), 1, 2, 3);
}

void test_range() {
  std::vector<char> hello = {'h', 'e', 'l', 'l', 'o'};
  (void)lll::fmt::format(LAWS3_FMT_STRING("{}"), hello);
}

int main() {
  test_format_api();
  test_chrono();
  test_text_style();
  test_range();
}
