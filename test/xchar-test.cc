// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.hpp.

#include "3laws/fmt/xchar.hpp"

#include <algorithm>
#include <complex>
#include <cwchar>
#include <vector>

#include "3laws/fmt/chrono.hpp"
#include "3laws/fmt/color.hpp"
#include "3laws/fmt/ostream.hpp"
#include "3laws/fmt/ranges.hpp"
#include "3laws/fmt/std.hpp"
#include "gtest-extra.h"  // Contains
#include "util.h"         // get_locale

using lll::fmt::detail::max_value;
using testing::Contains;

#if defined(__MINGW32__) && !defined(_UCRT)
// Only C89 conversion specifiers when using MSVCRT instead of UCRT
#  define LAWS3_FMT_HAS_C99_STRFTIME 0
#else
#  define LAWS3_FMT_HAS_C99_STRFTIME 1
#endif

struct non_string {};

template <typename T> class has_to_string_view_test : public testing::Test {};

using string_char_types = testing::Types<char, wchar_t, char16_t, char32_t>;
TYPED_TEST_SUITE(has_to_string_view_test, string_char_types);

template <typename Char>
struct derived_from_string_view : lll::fmt::basic_string_view<Char> {};

TYPED_TEST(has_to_string_view_test, has_to_string_view) {
  EXPECT_TRUE(lll::fmt::detail::has_to_string_view<TypeParam*>::value);
  EXPECT_TRUE(lll::fmt::detail::has_to_string_view<const TypeParam*>::value);
  EXPECT_TRUE(lll::fmt::detail::has_to_string_view<TypeParam[2]>::value);
  EXPECT_TRUE(lll::fmt::detail::has_to_string_view<const TypeParam[2]>::value);
  EXPECT_TRUE(lll::fmt::detail::has_to_string_view<
              std::basic_string<TypeParam>>::value);
  EXPECT_TRUE(lll::fmt::detail::has_to_string_view<
              lll::fmt::basic_string_view<TypeParam>>::value);
  EXPECT_TRUE(lll::fmt::detail::has_to_string_view<
              derived_from_string_view<TypeParam>>::value);
  using fmt_string_view = lll::fmt::detail::std_string_view<TypeParam>;
  EXPECT_TRUE(std::is_empty<fmt_string_view>::value !=
              lll::fmt::detail::has_to_string_view<fmt_string_view>::value);
  EXPECT_FALSE(lll::fmt::detail::has_to_string_view<non_string>::value);
}

// std::is_constructible is broken in MSVC until version 2015.
#if !LAWS3_FMT_MSC_VERSION || LAWS3_FMT_MSC_VERSION >= 1900
struct explicitly_convertible_to_wstring_view {
  explicit operator lll::fmt::wstring_view() const { return L"foo"; }
};

TEST(xchar_test, format_explicitly_convertible_to_wstring_view) {
  // Types explicitly convertible to wstring_view are not formattable by
  // default because it may introduce ODR violations.
  static_assert(
      !lll::fmt::is_formattable<explicitly_convertible_to_wstring_view>::value,
      "");
}
#endif

TEST(xchar_test, format) {
  EXPECT_EQ(L"42", lll::fmt::format(L"{}", 42));
  EXPECT_EQ(L"4.2", lll::fmt::format(L"{}", 4.2));
  EXPECT_EQ(L"abc", lll::fmt::format(L"{}", L"abc"));
  EXPECT_EQ(L"z", lll::fmt::format(L"{}", L'z'));
  EXPECT_THROW(lll::fmt::format(lll::fmt::runtime(L"{:*\x343E}"), 42),
               lll::fmt::format_error);
  EXPECT_EQ(L"true", lll::fmt::format(L"{}", true));
  EXPECT_EQ(L"a", lll::fmt::format(L"{0}", 'a'));
  EXPECT_EQ(L"a", lll::fmt::format(L"{0}", L'a'));
  EXPECT_EQ(L"Cyrillic letter \x42e",
            lll::fmt::format(L"Cyrillic letter {}", L'\x42e'));
  EXPECT_EQ(L"abc1", lll::fmt::format(L"{}c{}", L"ab", 1));
}

TEST(xchar_test, is_formattable) {
  static_assert(!lll::fmt::is_formattable<const wchar_t*>::value, "");
}

TEST(xchar_test, compile_time_string) {
  EXPECT_EQ(lll::fmt::format(lll::fmt::wformat_string<int>(L"{}"), 42), L"42");
#if defined(LAWS3_FMT_USE_STRING_VIEW) && LAWS3_FMT_CPLUSPLUS >= 201703L
  EXPECT_EQ(lll::fmt::format(LAWS3_FMT_STRING(std::wstring_view(L"{}")), 42),
            L"42");
#endif
}

#if LAWS3_FMT_CPLUSPLUS > 201103L
struct custom_char {
  int value;
  custom_char() = default;

  template <typename T>
  constexpr custom_char(T val) : value(static_cast<int>(val)) {}

  constexpr operator char() const {
    return value <= 0xff ? static_cast<char>(value) : '\0';
  }
  constexpr bool operator<(custom_char c) const { return value < c.value; }
};

namespace std {

template <> struct char_traits<custom_char> {
  using char_type = custom_char;
  using int_type = int;
  using off_type = streamoff;
  using pos_type = streampos;
  using state_type = mbstate_t;

  static constexpr void assign(char_type& r, const char_type& a) { r = a; }
  static constexpr bool eq(char_type a, char_type b) { return a == b; }
  static constexpr bool lt(char_type a, char_type b) { return a < b; }
  static LAWS3_FMT_CONSTEXPR int compare(const char_type* s1,
                                         const char_type* s2, size_t count) {
    for (; count; count--, s1++, s2++) {
      if (lt(*s1, *s2)) return -1;
      if (lt(*s2, *s1)) return 1;
    }
    return 0;
  }
  static LAWS3_FMT_CONSTEXPR size_t length(const char_type* s) {
    size_t count = 0;
    while (!eq(*s++, custom_char(0))) count++;
    return count;
  }
  static const char_type* find(const char_type*, size_t, const char_type&);
  static LAWS3_FMT_CONSTEXPR char_type* move(char_type* dest,
                                             const char_type* src,
                                             size_t count) {
    if (count == 0) return dest;
    char_type* ret = dest;
    if (src < dest) {
      dest += count;
      src += count;
      for (; count; count--) assign(*--dest, *--src);
    } else if (src > dest)
      copy(dest, src, count);
    return ret;
  }
  static LAWS3_FMT_CONSTEXPR char_type* copy(char_type* dest,
                                             const char_type* src,
                                             size_t count) {
    char_type* ret = dest;
    for (; count; count--) assign(*dest++, *src++);
    return ret;
  }
  static LAWS3_FMT_CONSTEXPR char_type* assign(char_type* dest,
                                               std::size_t count, char_type a) {
    char_type* ret = dest;
    for (; count; count--) assign(*dest++, a);
    return ret;
  }
  static int_type not_eof(int_type);
  static char_type to_char_type(int_type);
  static int_type to_int_type(char_type);
  static bool eq_int_type(int_type, int_type);
  static int_type eof();
};

}  // namespace std

auto to_ascii(custom_char c) -> char { return c; }

LAWS3_FMT_BEGIN_NAMESPACE
template <> struct is_char<custom_char> : std::true_type {};
LAWS3_FMT_END_NAMESPACE

TEST(xchar_test, format_custom_char) {
  const custom_char format[] = {'{', '}', 0};
  auto result = lll::fmt::format(format, custom_char('x'));
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], custom_char('x'));
}
#endif

// Convert a char8_t string to std::string. Otherwise GTest will insist on
// inserting `char8_t` NTBS into a `char` stream which is disabled by P1423.
template <typename S> std::string from_u8str(const S& str) {
  return std::string(str.begin(), str.end());
}

TEST(xchar_test, format_to) {
  auto buf = std::vector<wchar_t>();
  lll::fmt::format_to(std::back_inserter(buf), L"{}{}", 42, L'\0');
  EXPECT_STREQ(buf.data(), L"42");
}

TEST(xchar_test, vformat_to) {
  int n = 42;
  auto args = lll::fmt::make_wformat_args(n);
  auto w = std::wstring();
  lll::fmt::vformat_to(std::back_inserter(w), L"{}", args);
  EXPECT_EQ(L"42", w);
}

namespace test {
struct struct_as_wstring_view {};
auto format_as(struct_as_wstring_view) -> lll::fmt::wstring_view {
  return L"foo";
}
}  // namespace test

TEST(xchar_test, format_as) {
  EXPECT_EQ(lll::fmt::format(L"{}", test::struct_as_wstring_view()), L"foo");
}

TEST(format_test, wide_format_to_n) {
  wchar_t buffer[4];
  buffer[3] = L'x';
  auto result = lll::fmt::format_to_n(buffer, 3, L"{}", 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"123x", lll::fmt::wstring_view(buffer, 4));
  buffer[0] = L'x';
  buffer[1] = L'x';
  buffer[2] = L'x';
  result = lll::fmt::format_to_n(buffer, 3, L"{}", L'A');
  EXPECT_EQ(1u, result.size);
  EXPECT_EQ(buffer + 1, result.out);
  EXPECT_EQ(L"Axxx", lll::fmt::wstring_view(buffer, 4));
  result = lll::fmt::format_to_n(buffer, 3, L"{}{} ", L'B', L'C');
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"BC x", lll::fmt::wstring_view(buffer, 4));
}

#if LAWS3_FMT_USE_USER_DEFINED_LITERALS
TEST(xchar_test, named_arg_udl) {
  using namespace lll::fmt::literals;
  auto udl_a =
      lll::fmt::format(L"{first}{second}{first}{third}", L"first"_a = L"abra",
                       L"second"_a = L"cad", L"third"_a = 99);
  EXPECT_EQ(lll::fmt::format(L"{first}{second}{first}{third}",
                             lll::fmt::arg(L"first", L"abra"),
                             lll::fmt::arg(L"second", L"cad"),
                             lll::fmt::arg(L"third", 99)),
            udl_a);
}
#endif  // LAWS3_FMT_USE_USER_DEFINED_LITERALS

TEST(xchar_test, print) {
  // Check that the wide print overload compiles.
  if (lll::fmt::detail::const_check(false)) {
    lll::fmt::print(L"test");
    lll::fmt::println(L"test");
  }
}

TEST(xchar_test, join) {
  int v[3] = {1, 2, 3};
  EXPECT_EQ(lll::fmt::format(L"({})", lll::fmt::join(v, v + 3, L", ")),
            L"(1, 2, 3)");
  auto t = std::tuple<wchar_t, int, float>('a', 1, 2.0f);
  EXPECT_EQ(lll::fmt::format(L"({})", lll::fmt::join(t, L", ")), L"(a, 1, 2)");
}

enum streamable_enum {};

std::wostream& operator<<(std::wostream& os, streamable_enum) {
  return os << L"streamable_enum";
}

namespace lll {
namespace fmt {
template <>
struct formatter<streamable_enum, wchar_t> : basic_ostream_formatter<wchar_t> {
};
}  // namespace fmt
}  // namespace lll

enum unstreamable_enum {};
auto format_as(unstreamable_enum e) -> int { return e; }

TEST(xchar_test, enum) {
  EXPECT_EQ(L"streamable_enum", lll::fmt::format(L"{}", streamable_enum()));
  EXPECT_EQ(L"0", lll::fmt::format(L"{}", unstreamable_enum()));
}

struct streamable_and_unformattable {};

auto operator<<(std::wostream& os, streamable_and_unformattable)
    -> std::wostream& {
  return os << L"foo";
}

TEST(xchar_test, streamed) {
  EXPECT_FALSE(lll::fmt::is_formattable<streamable_and_unformattable>());
  EXPECT_EQ(lll::fmt::format(
                L"{}", lll::fmt::streamed(streamable_and_unformattable())),
            L"foo");
}

TEST(xchar_test, sign_not_truncated) {
  wchar_t format_str[] = {
      L'{', L':',
      '+' | static_cast<wchar_t>(1 << lll::fmt::detail::num_bits<char>()), L'}',
      0};
  EXPECT_THROW(lll::fmt::format(lll::fmt::runtime(format_str), 42),
               lll::fmt::format_error);
}

TEST(xchar_test, chrono) {
  auto tm = std::tm();
  tm.tm_year = 116;
  tm.tm_mon = 3;
  tm.tm_mday = 25;
  tm.tm_hour = 11;
  tm.tm_min = 22;
  tm.tm_sec = 33;
  EXPECT_EQ(lll::fmt::format("The date is {:%Y-%m-%d %H:%M:%S}.", tm),
            "The date is 2016-04-25 11:22:33.");
  EXPECT_EQ(L"42s", lll::fmt::format(L"{}", std::chrono::seconds(42)));
  EXPECT_EQ(lll::fmt::format(L"{:%F}", tm), L"2016-04-25");
  EXPECT_EQ(lll::fmt::format(L"{:%T}", tm), L"11:22:33");
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
  auto t1 = std::chrono::time_point_cast<std::chrono::seconds>(
      std::chrono::system_clock::now());

  std::vector<std::wstring> spec_list = {
      L"%%",  L"%n",  L"%t",  L"%Y",  L"%EY", L"%y",  L"%Oy", L"%Ey", L"%C",
      L"%EC", L"%G",  L"%g",  L"%b",  L"%h",  L"%B",  L"%m",  L"%Om", L"%U",
      L"%OU", L"%W",  L"%OW", L"%V",  L"%OV", L"%j",  L"%d",  L"%Od", L"%e",
      L"%Oe", L"%a",  L"%A",  L"%w",  L"%Ow", L"%u",  L"%Ou", L"%H",  L"%OH",
      L"%I",  L"%OI", L"%M",  L"%OM", L"%S",  L"%OS", L"%x",  L"%Ex", L"%X",
      L"%EX", L"%D",  L"%F",  L"%R",  L"%T",  L"%p"};
#ifndef _WIN32
  // Disabled on Windows, because these formats is not consistent among
  // platforms.
  spec_list.insert(spec_list.end(), {L"%c", L"%Ec", L"%r"});
#elif !LAWS3_FMT_HAS_C99_STRFTIME
  // Only C89 conversion specifiers when using MSVCRT instead of UCRT
  spec_list = {L"%%", L"%Y", L"%y", L"%b", L"%B", L"%m", L"%U",
               L"%W", L"%j", L"%d", L"%a", L"%A", L"%w", L"%H",
               L"%I", L"%M", L"%S", L"%x", L"%X", L"%p"};
#endif
  spec_list.push_back(L"%Y-%m-%d %H:%M:%S");

  for (const auto& spec : spec_list) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::gmtime(&t);

    auto sys_output = system_wcsftime(spec, &tm);

    auto fmt_spec = lll::fmt::format(L"{{:{}}}", spec);
    EXPECT_EQ(sys_output, lll::fmt::format(lll::fmt::runtime(fmt_spec), t1));
    EXPECT_EQ(sys_output, lll::fmt::format(lll::fmt::runtime(fmt_spec), tm));
  }

  // Timezone formatters tests makes sense for localtime.
#if LAWS3_FMT_HAS_C99_STRFTIME
  spec_list = {L"%z", L"%Z"};
#else
  spec_list = {L"%Z"};
#endif
  for (const auto& spec : spec_list) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::localtime(&t);

    auto sys_output = system_wcsftime(spec, &tm);

    auto fmt_spec = lll::fmt::format(L"{{:{}}}", spec);
    EXPECT_EQ(sys_output, lll::fmt::format(lll::fmt::runtime(fmt_spec), tm));

    if (spec == L"%z") {
      sys_output.insert(sys_output.end() - 2, 1, L':');
      EXPECT_EQ(sys_output, lll::fmt::format(L"{:%Ez}", tm));
      EXPECT_EQ(sys_output, lll::fmt::format(L"{:%Oz}", tm));
    }
  }

  // Separate tests for UTC, since std::time_put can use local time and ignoring
  // the timezone in std::tm (if it presents on platform).
  if (lll::fmt::detail::has_member_data_tm_zone<std::tm>::value) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::gmtime(&t);

    std::vector<std::wstring> tz_names = {L"GMT", L"UTC"};
    EXPECT_THAT(tz_names, Contains(lll::fmt::format(L"{:%Z}", t1)));
    EXPECT_THAT(tz_names, Contains(lll::fmt::format(L"{:%Z}", tm)));
  }

  if (lll::fmt::detail::has_member_data_tm_gmtoff<std::tm>::value) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::gmtime(&t);

    EXPECT_EQ(L"+0000", lll::fmt::format(L"{:%z}", t1));
    EXPECT_EQ(L"+0000", lll::fmt::format(L"{:%z}", tm));

    EXPECT_EQ(L"+00:00", lll::fmt::format(L"{:%Ez}", t1));
    EXPECT_EQ(L"+00:00", lll::fmt::format(L"{:%Ez}", tm));

    EXPECT_EQ(L"+00:00", lll::fmt::format(L"{:%Oz}", t1));
    EXPECT_EQ(L"+00:00", lll::fmt::format(L"{:%Oz}", tm));
  }
}

TEST(xchar_test, color) {
  EXPECT_EQ(
      lll::fmt::format(fg(lll::fmt::rgb(255, 20, 30)), L"rgb(255,20,30) wide"),
      L"\x1b[38;2;255;020;030mrgb(255,20,30) wide\x1b[0m");
}

TEST(xchar_test, ostream) {
#if !LAWS3_FMT_GCC_VERSION || LAWS3_FMT_GCC_VERSION >= 409
  {
    std::wostringstream wos;
    lll::fmt::print(wos, L"Don't {}!", L"panic");
    EXPECT_EQ(wos.str(), L"Don't panic!");
  }

  {
    std::wostringstream wos;
    lll::fmt::println(wos, L"Don't {}!", L"panic");
    EXPECT_EQ(wos.str(), L"Don't panic!\n");
  }
#endif
}

TEST(xchar_test, format_map) {
  auto m = std::map<std::wstring, int>{{L"one", 1}, {L"t\"wo", 2}};
  EXPECT_EQ(lll::fmt::format(L"{}", m), L"{\"one\": 1, \"t\\\"wo\": 2}");
}

TEST(xchar_test, escape_string) {
  using vec = std::vector<std::wstring>;
  EXPECT_EQ(lll::fmt::format(L"{}", vec{L"\n\r\t\"\\"}),
            L"[\"\\n\\r\\t\\\"\\\\\"]");
  EXPECT_EQ(lll::fmt::format(L"{}", vec{L"понедельник"}), L"[\"понедельник\"]");
}

TEST(xchar_test, to_wstring) { EXPECT_EQ(L"42", lll::fmt::to_wstring(42)); }

#ifndef LAWS3_FMT_STATIC_THOUSANDS_SEPARATOR

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
  EXPECT_EQ(lll::fmt::format(loc, "{:L}", 1.23), "1?23");
  EXPECT_EQ(lll::fmt::format(loc, "{:Lf}", 1.23), "1?230000");
  EXPECT_EQ(lll::fmt::format(loc, "{:L}", 1234.5), "1~234?5");
  EXPECT_EQ(lll::fmt::format(loc, "{:L}", 12000.0), "12~000");
  EXPECT_EQ(lll::fmt::format(loc, "{:8L}", 1230.0), "   1~230");
  EXPECT_EQ(lll::fmt::format(loc, "{:15.6Lf}", 0.1), "       0?100000");
  EXPECT_EQ(lll::fmt::format(loc, "{:15.6Lf}", 1.0), "       1?000000");
  EXPECT_EQ(lll::fmt::format(loc, "{:15.6Lf}", 1e3), "   1~000?000000");
}

TEST(locale_test, format) {
  auto loc = std::locale(std::locale(), new numpunct<char>());
  EXPECT_EQ("1234567", lll::fmt::format(std::locale(), "{:L}", 1234567));
  EXPECT_EQ("1~234~567", lll::fmt::format(loc, "{:L}", 1234567));
  EXPECT_EQ("-1~234~567", lll::fmt::format(loc, "{:L}", -1234567));
  EXPECT_EQ("-256", lll::fmt::format(loc, "{:L}", -256));
  auto n = 1234567;
  EXPECT_EQ("1~234~567",
            lll::fmt::vformat(loc, "{:L}", lll::fmt::make_format_args(n)));
  auto s = std::string();
  lll::fmt::format_to(std::back_inserter(s), loc, "{:L}", 1234567);
  EXPECT_EQ("1~234~567", s);

  auto no_grouping_loc = std::locale(std::locale(), new no_grouping<char>());
  EXPECT_EQ("1234567", lll::fmt::format(no_grouping_loc, "{:L}", 1234567));

  auto special_grouping_loc =
      std::locale(std::locale(), new special_grouping<char>());
  EXPECT_EQ("1,23,45,678",
            lll::fmt::format(special_grouping_loc, "{:L}", 12345678));
  EXPECT_EQ("12,345", lll::fmt::format(special_grouping_loc, "{:L}", 12345));

  auto small_grouping_loc =
      std::locale(std::locale(), new small_grouping<char>());
  EXPECT_EQ("4,2,9,4,9,6,7,2,9,5", lll::fmt::format(small_grouping_loc, "{:L}",
                                                    max_value<uint32_t>()));
}

TEST(locale_test, format_detault_align) {
  auto loc = std::locale({}, new special_grouping<char>());
  EXPECT_EQ("  12,345", lll::fmt::format(loc, "{:8L}", 12345));
}

TEST(locale_test, format_plus) {
  auto loc = std::locale({}, new special_grouping<char>());
  EXPECT_EQ("+100", lll::fmt::format(loc, "{:+L}", 100));
}

TEST(locale_test, wformat) {
  auto loc = std::locale(std::locale(), new numpunct<wchar_t>());
  EXPECT_EQ(L"1234567", lll::fmt::format(std::locale(), L"{:L}", 1234567));
  EXPECT_EQ(L"1~234~567", lll::fmt::format(loc, L"{:L}", 1234567));
  int n = 1234567;
  EXPECT_EQ(L"1~234~567",
            lll::fmt::vformat(loc, L"{:L}", lll::fmt::make_wformat_args(n)));
  EXPECT_EQ(L"1234567", lll::fmt::format(std::locale("C"), L"{:L}", 1234567));

  auto no_grouping_loc = std::locale(std::locale(), new no_grouping<wchar_t>());
  EXPECT_EQ(L"1234567", lll::fmt::format(no_grouping_loc, L"{:L}", 1234567));

  auto special_grouping_loc =
      std::locale(std::locale(), new special_grouping<wchar_t>());
  EXPECT_EQ(L"1,23,45,678",
            lll::fmt::format(special_grouping_loc, L"{:L}", 12345678));

  auto small_grouping_loc =
      std::locale(std::locale(), new small_grouping<wchar_t>());
  EXPECT_EQ(
      L"4,2,9,4,9,6,7,2,9,5",
      lll::fmt::format(small_grouping_loc, L"{:L}", max_value<uint32_t>()));
}

TEST(locale_test, int_formatter) {
  auto loc = std::locale(std::locale(), new special_grouping<char>());
  auto f = lll::fmt::formatter<int>();
  auto parse_ctx = lll::fmt::format_parse_context("L");
  f.parse(parse_ctx);
  auto buf = lll::fmt::memory_buffer();
  lll::fmt::basic_format_context<lll::fmt::appender, char> format_ctx(
      lll::fmt::appender(buf), {}, lll::fmt::detail::locale_ref(loc));
  f.format(12345, format_ctx);
  EXPECT_EQ(lll::fmt::to_string(buf), "12,345");
}

LAWS3_FMT_BEGIN_NAMESPACE
template <class charT> struct formatter<std::complex<double>, charT> {
 private:
  detail::dynamic_format_specs<char> specs_;

 public:
  LAWS3_FMT_CONSTEXPR typename basic_format_parse_context<charT>::iterator
  parse(basic_format_parse_context<charT>& ctx) {
    auto end = parse_format_specs(ctx.begin(), ctx.end(), specs_, ctx,
                                  detail::type::float_type);
    detail::parse_float_type_spec(specs_);
    return end;
  }

  template <class FormatContext>
  typename FormatContext::iterator format(const std::complex<double>& c,
                                          FormatContext& ctx) const {
    auto specs = specs_;
    detail::handle_dynamic_spec<detail::precision_checker>(
        specs.precision, specs.precision_ref, ctx);
    auto fspecs = std::string();
    if (specs.precision > 0) fspecs = lll::fmt::format(".{}", specs.precision);
    if (specs.type == presentation_type::fixed) fspecs += 'f';
    auto real =
        lll::fmt::format(ctx.locale().template get<std::locale>(),
                         lll::fmt::runtime("{:" + fspecs + "}"), c.real());
    auto imag =
        lll::fmt::format(ctx.locale().template get<std::locale>(),
                         lll::fmt::runtime("{:" + fspecs + "}"), c.imag());
    auto fill_align_width = std::string();
    if (specs.width > 0)
      fill_align_width = lll::fmt::format(">{}", specs.width);
    return lll::fmt::format_to(
        ctx.out(), runtime("{:" + fill_align_width + "}"),
        c.real() != 0 ? lll::fmt::format("({}+{}i)", real, imag)
                      : lll::fmt::format("{}i", imag));
  }
};
LAWS3_FMT_END_NAMESPACE

TEST(locale_test, complex) {
  std::string s = lll::fmt::format("{}", std::complex<double>(1, 2));
  EXPECT_EQ(s, "(1+2i)");
  EXPECT_EQ(lll::fmt::format("{:.2f}", std::complex<double>(1, 2)),
            "(1.00+2.00i)");
  EXPECT_EQ(lll::fmt::format("{:8}", std::complex<double>(1, 2)), "  (1+2i)");
}

TEST(locale_test, chrono_weekday) {
  auto loc = get_locale("es_ES.UTF-8", "Spanish_Spain.1252");
  auto loc_old = std::locale::global(loc);
  auto sat = lll::fmt::weekday(6);
  EXPECT_EQ(lll::fmt::format(L"{}", sat), L"Sat");
  if (loc != std::locale::classic()) {
    // L'\xE1' is 'á'.
    auto saturdays = std::vector<std::wstring>{
        L"s\xE1"
        "b",
        L"s\xE1."};
    EXPECT_THAT(saturdays, Contains(lll::fmt::format(loc, L"{:L}", sat)));
  }
  std::locale::global(loc_old);
}

TEST(locale_test, sign) {
  EXPECT_EQ(lll::fmt::format(std::locale(), L"{:L}", -50), L"-50");
}

TEST(std_test_xchar, optional) {
#  ifdef __cpp_lib_optional
  EXPECT_EQ(lll::fmt::format(L"{}", std::optional{L'C'}), L"optional(\'C\')");
  EXPECT_EQ(
      lll::fmt::format(L"{}", std::optional{std::wstring{L"wide string"}}),
      L"optional(\"wide string\")");
#  endif
}

#endif  // LAWS3_FMT_STATIC_THOUSANDS_SEPARATOR
