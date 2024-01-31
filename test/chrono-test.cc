// Formatting library for C++ - time formatting tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "3laws/fmt/chrono.h"

#include <algorithm>
#include <ctime>
#include <vector>

#include "gtest-extra.h"  // EXPECT_THROW_MSG
#include "util.h"         // get_locale

using lll::fmt::runtime;
using testing::Contains;

template <typename Duration>
using sys_time = std::chrono::time_point<std::chrono::system_clock, Duration>;

#if defined(__MINGW32__) && !defined(_UCRT)
// Only C89 conversion specifiers when using MSVCRT instead of UCRT
#  define LAWS3_FMT_HAS_C99_STRFTIME 0
#else
#  define LAWS3_FMT_HAS_C99_STRFTIME 1
#endif

#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
using days = std::chrono::days;
#else
using days = std::chrono::duration<std::chrono::hours::rep, std::ratio<86400>>;
#endif

auto make_tm() -> std::tm {
  auto time = std::tm();
  time.tm_mday = 1;
  return time;
}

auto make_hour(int h) -> std::tm {
  auto time = make_tm();
  time.tm_hour = h;
  return time;
}

auto make_minute(int m) -> std::tm {
  auto time = make_tm();
  time.tm_min = m;
  return time;
}

auto make_second(int s) -> std::tm {
  auto time = make_tm();
  time.tm_sec = s;
  return time;
}

std::string system_strftime(const std::string& format, const std::tm* timeptr,
                            std::locale* locptr = nullptr) {
  auto loc = locptr ? *locptr : std::locale::classic();
  auto& facet = std::use_facet<std::time_put<char>>(loc);
  std::ostringstream os;
  os.imbue(loc);
  facet.put(os, os, ' ', timeptr, format.c_str(),
            format.c_str() + format.size());
#ifdef _WIN32
  // Workaround a bug in older versions of Universal CRT.
  auto str = os.str();
  if (str == "-0000") str = "+0000";
  return str;
#else
  return os.str();
#endif
}

LAWS3_FMT_CONSTEXPR std::tm make_tm(int year, int mon, int mday, int hour,
                                    int min, int sec) {
  auto tm = std::tm();
  tm.tm_sec = sec;
  tm.tm_min = min;
  tm.tm_hour = hour;
  tm.tm_mday = mday;
  tm.tm_mon = mon - 1;
  tm.tm_year = year - 1900;
  return tm;
}

TEST(chrono_test, format_tm) {
  auto tm = std::tm();
  tm.tm_year = 116;
  tm.tm_mon = 3;
  tm.tm_mday = 25;
  tm.tm_hour = 11;
  tm.tm_min = 22;
  tm.tm_sec = 33;
  EXPECT_EQ(lll::fmt::format("The date is {:%Y-%m-%d %H:%M:%S}.", tm),
            "The date is 2016-04-25 11:22:33.");
  EXPECT_EQ(lll::fmt::format("{:%Y}", tm), "2016");
  EXPECT_EQ(lll::fmt::format("{:%C}", tm), "20");
  EXPECT_EQ(lll::fmt::format("{:%C%y}", tm), lll::fmt::format("{:%Y}", tm));
  EXPECT_EQ(lll::fmt::format("{:%e}", tm), "25");
  EXPECT_EQ(lll::fmt::format("{:%D}", tm), "04/25/16");
  EXPECT_EQ(lll::fmt::format("{:%F}", tm), "2016-04-25");
  EXPECT_EQ(lll::fmt::format("{:%T}", tm), "11:22:33");

  // Short year
  tm.tm_year = 999 - 1900;
  tm.tm_mon = 0;   // for %G
  tm.tm_mday = 2;  // for %G
  tm.tm_wday = 3;  // for %G
  tm.tm_yday = 1;  // for %G
  EXPECT_EQ(lll::fmt::format("{:%Y}", tm), "0999");
  EXPECT_EQ(lll::fmt::format("{:%C%y}", tm), "0999");
  EXPECT_EQ(lll::fmt::format("{:%G}", tm), "0999");

  tm.tm_year = 27 - 1900;
  EXPECT_EQ(lll::fmt::format("{:%Y}", tm), "0027");
  EXPECT_EQ(lll::fmt::format("{:%C%y}", tm), "0027");

  // Overflow year
  tm.tm_year = 2147483647;
  EXPECT_EQ(lll::fmt::format("{:%Y}", tm), "2147485547");

  tm.tm_year = -2147483648;
  EXPECT_EQ(lll::fmt::format("{:%Y}", tm), "-2147481748");

  // for week on the year
  // https://www.cl.cam.ac.uk/~mgk25/iso-time.html
  std::vector<std::tm> tm_list = {
      make_tm(1975, 12, 29, 12, 14, 16),  // W01
      make_tm(1977, 1, 2, 12, 14, 16),    // W53
      make_tm(1999, 12, 27, 12, 14, 16),  // W52
      make_tm(1999, 12, 31, 12, 14, 16),  // W52
      make_tm(2000, 1, 1, 12, 14, 16),    // W52
      make_tm(2000, 1, 2, 12, 14, 16),    // W52
      make_tm(2000, 1, 3, 12, 14, 16)     // W1
  };

#if !LAWS3_FMT_HAS_C99_STRFTIME
  GTEST_SKIP() << "Skip the rest of this test because it relies on strftime() "
                  "conforming to C99, but on this platform, MINGW + MSVCRT, "
                  "the function conforms only to C89.";
#endif

  const std::string iso_week_spec = "%Y-%m-%d: %G %g %V";
  for (auto ctm : tm_list) {
    // Calculate tm_yday, tm_wday, etc.
    std::time_t t = std::mktime(&ctm);
    tm = *std::localtime(&t);

    auto fmt_spec = lll::fmt::format("{{:{}}}", iso_week_spec);
    EXPECT_EQ(system_strftime(iso_week_spec, &tm),
              lll::fmt::format(lll::fmt::runtime(fmt_spec), tm));
  }

  // Every day from 1970-01-01
  std::time_t time_now = std::time(nullptr);
  for (std::time_t t = 6 * 3600; t < time_now; t += 86400) {
    tm = *std::localtime(&t);

    auto fmt_spec = lll::fmt::format("{{:{}}}", iso_week_spec);
    EXPECT_EQ(system_strftime(iso_week_spec, &tm),
              lll::fmt::format(lll::fmt::runtime(fmt_spec), tm));
  }
}

// MSVC:
//  minkernel\crts\ucrt\src\appcrt\time\wcsftime.cpp(971) : Assertion failed:
//  timeptr->tm_year >= -1900 && timeptr->tm_year <= 8099
#ifndef _WIN32
TEST(chrono_test, format_tm_future) {
  auto tm = std::tm();
  tm.tm_year = 10445;  // 10000+ years
  tm.tm_mon = 3;
  tm.tm_mday = 25;
  tm.tm_hour = 11;
  tm.tm_min = 22;
  tm.tm_sec = 33;
  EXPECT_EQ(lll::fmt::format("The date is {:%Y-%m-%d %H:%M:%S}.", tm),
            "The date is 12345-04-25 11:22:33.");
  EXPECT_EQ(lll::fmt::format("{:%Y}", tm), "12345");
  EXPECT_EQ(lll::fmt::format("{:%C}", tm), "123");
  EXPECT_EQ(lll::fmt::format("{:%C%y}", tm), lll::fmt::format("{:%Y}", tm));
  EXPECT_EQ(lll::fmt::format("{:%D}", tm), "04/25/45");
  EXPECT_EQ(lll::fmt::format("{:%F}", tm), "12345-04-25");
  EXPECT_EQ(lll::fmt::format("{:%T}", tm), "11:22:33");
}

TEST(chrono_test, format_tm_past) {
  auto tm = std::tm();
  tm.tm_year = -2001;
  tm.tm_mon = 3;
  tm.tm_mday = 25;
  tm.tm_hour = 11;
  tm.tm_min = 22;
  tm.tm_sec = 33;
  EXPECT_EQ(lll::fmt::format("The date is {:%Y-%m-%d %H:%M:%S}.", tm),
            "The date is -101-04-25 11:22:33.");
  EXPECT_EQ(lll::fmt::format("{:%Y}", tm), "-101");

  // macOS  %C - "-1"
  // Linux  %C - "-2"
  // fmt    %C - "-1"
  EXPECT_EQ(lll::fmt::format("{:%C}", tm), "-1");
  EXPECT_EQ(lll::fmt::format("{:%C%y}", tm), lll::fmt::format("{:%Y}", tm));

  // macOS  %D - "04/25/01" (%y)
  // Linux  %D - "04/25/99" (%y)
  // fmt    %D - "04/25/01" (%y)
  EXPECT_EQ(lll::fmt::format("{:%D}", tm), "04/25/01");

  EXPECT_EQ(lll::fmt::format("{:%F}", tm), "-101-04-25");
  EXPECT_EQ(lll::fmt::format("{:%T}", tm), "11:22:33");

  tm.tm_year = -1901;  // -1
  EXPECT_EQ(lll::fmt::format("{:%Y}", tm), "-001");
  EXPECT_EQ(lll::fmt::format("{:%C%y}", tm), lll::fmt::format("{:%Y}", tm));

  tm.tm_year = -1911;  // -11
  EXPECT_EQ(lll::fmt::format("{:%Y}", tm), "-011");
  EXPECT_EQ(lll::fmt::format("{:%C%y}", tm), lll::fmt::format("{:%Y}", tm));
}
#endif

TEST(chrono_test, grow_buffer) {
  auto s = std::string("{:");
  for (int i = 0; i < 30; ++i) s += "%c";
  s += "}\n";
  auto t = std::time(nullptr);
  (void)lll::fmt::format(lll::fmt::runtime(s), *std::localtime(&t));
}

TEST(chrono_test, format_to_empty_container) {
  auto time = std::tm();
  time.tm_sec = 42;
  auto s = std::string();
  lll::fmt::format_to(std::back_inserter(s), "{:%S}", time);
  EXPECT_EQ(s, "42");
}

TEST(chrono_test, empty_result) {
  EXPECT_EQ(lll::fmt::format("{}", std::tm()), "");
}

auto equal(const std::tm& lhs, const std::tm& rhs) -> bool {
  return lhs.tm_sec == rhs.tm_sec && lhs.tm_min == rhs.tm_min &&
         lhs.tm_hour == rhs.tm_hour && lhs.tm_mday == rhs.tm_mday &&
         lhs.tm_mon == rhs.tm_mon && lhs.tm_year == rhs.tm_year &&
         lhs.tm_wday == rhs.tm_wday && lhs.tm_yday == rhs.tm_yday &&
         lhs.tm_isdst == rhs.tm_isdst;
}

TEST(chrono_test, gmtime) {
  auto t = std::time(nullptr);
  auto tm = *std::gmtime(&t);
  EXPECT_TRUE(equal(tm, lll::fmt::gmtime(t)));
}

template <typename TimePoint>
auto strftime_full_utc(TimePoint tp) -> std::string {
  auto t = std::chrono::system_clock::to_time_t(tp);
  auto tm = *std::gmtime(&t);
  return system_strftime("%Y-%m-%d %H:%M:%S", &tm);
}

TEST(chrono_test, system_clock_time_point) {
  auto t1 = std::chrono::time_point_cast<std::chrono::seconds>(
      std::chrono::system_clock::now());
  EXPECT_EQ(strftime_full_utc(t1),
            lll::fmt::format("{:%Y-%m-%d %H:%M:%S}", t1));
  EXPECT_EQ(strftime_full_utc(t1), lll::fmt::format("{}", t1));
  EXPECT_EQ(strftime_full_utc(t1), lll::fmt::format("{:}", t1));

  auto t2 = sys_time<std::chrono::seconds>(std::chrono::seconds(42));
  EXPECT_EQ(strftime_full_utc(t2),
            lll::fmt::format("{:%Y-%m-%d %H:%M:%S}", t2));

  std::vector<std::string> spec_list = {
      "%%",  "%n",  "%t",  "%Y",  "%EY", "%y",  "%Oy", "%Ey", "%C",
      "%EC", "%G",  "%g",  "%b",  "%h",  "%B",  "%m",  "%Om", "%U",
      "%OU", "%W",  "%OW", "%V",  "%OV", "%j",  "%d",  "%Od", "%e",
      "%Oe", "%a",  "%A",  "%w",  "%Ow", "%u",  "%Ou", "%H",  "%OH",
      "%I",  "%OI", "%M",  "%OM", "%S",  "%OS", "%x",  "%Ex", "%X",
      "%EX", "%D",  "%F",  "%R",  "%T",  "%p"};
#ifndef _WIN32
  // Disabled on Windows because these formats are not consistent among
  // platforms.
  spec_list.insert(spec_list.end(), {"%c", "%Ec", "%r"});
#elif !LAWS3_FMT_HAS_C99_STRFTIME
  // Only C89 conversion specifiers when using MSVCRT instead of UCRT
  spec_list = {"%%", "%Y", "%y", "%b", "%B", "%m", "%U", "%W", "%j", "%d",
               "%a", "%A", "%w", "%H", "%I", "%M", "%S", "%x", "%X", "%p"};
#endif
  spec_list.push_back("%Y-%m-%d %H:%M:%S");

  for (const auto& spec : spec_list) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::gmtime(&t);

    auto sys_output = system_strftime(spec, &tm);

    auto fmt_spec = lll::fmt::format("{{:{}}}", spec);
    EXPECT_EQ(sys_output, lll::fmt::format(lll::fmt::runtime(fmt_spec), t1));
    EXPECT_EQ(sys_output, lll::fmt::format(lll::fmt::runtime(fmt_spec), tm));
  }

  // Timezone formatters tests makes sense for localtime.
#if LAWS3_FMT_HAS_C99_STRFTIME
  spec_list = {"%z", "%Z"};
#else
  spec_list = {"%Z"};
#endif
  for (const auto& spec : spec_list) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::localtime(&t);

    auto sys_output = system_strftime(spec, &tm);

    auto fmt_spec = lll::fmt::format("{{:{}}}", spec);
    EXPECT_EQ(sys_output, lll::fmt::format(lll::fmt::runtime(fmt_spec), tm));

    if (spec == "%z") {
      sys_output.insert(sys_output.end() - 2, 1, ':');
      EXPECT_EQ(sys_output, lll::fmt::format("{:%Ez}", tm));
      EXPECT_EQ(sys_output, lll::fmt::format("{:%Oz}", tm));
    }
  }

  // Separate tests for UTC, since std::time_put can use local time and ignoring
  // the timezone in std::tm (if it presents on platform).
  if (lll::fmt::detail::has_member_data_tm_zone<std::tm>::value) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::gmtime(&t);

    std::vector<std::string> tz_names = {"GMT", "UTC"};
    EXPECT_THAT(tz_names, Contains(lll::fmt::format("{:%Z}", t1)));
    EXPECT_THAT(tz_names, Contains(lll::fmt::format("{:%Z}", tm)));
  }

  if (lll::fmt::detail::has_member_data_tm_gmtoff<std::tm>::value) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::gmtime(&t);

    EXPECT_EQ(lll::fmt::format("{:%z}", t1), "+0000");
    EXPECT_EQ(lll::fmt::format("{:%z}", tm), "+0000");

    EXPECT_EQ(lll::fmt::format("{:%Ez}", t1), "+00:00");
    EXPECT_EQ(lll::fmt::format("{:%Ez}", tm), "+00:00");

    EXPECT_EQ(lll::fmt::format("{:%Oz}", t1), "+00:00");
    EXPECT_EQ(lll::fmt::format("{:%Oz}", tm), "+00:00");
  }
}

#if LAWS3_FMT_USE_LOCAL_TIME

TEST(chrono_test, localtime) {
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  EXPECT_TRUE(equal(tm, lll::fmt::localtime(t)));
}

template <typename Duration>
auto strftime_full_local(std::chrono::local_time<Duration> tp) -> std::string {
  auto t = std::chrono::system_clock::to_time_t(
      std::chrono::current_zone()->to_sys(tp));
  auto tm = *std::localtime(&t);
  return system_strftime("%Y-%m-%d %H:%M:%S", &tm);
}

TEST(chrono_test, local_system_clock_time_point) {
#  ifdef _WIN32
  return;  // Not supported on Windows.
#  endif
  auto t1 = std::chrono::time_point_cast<std::chrono::seconds>(
      std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
  EXPECT_EQ(strftime_full_local(t1),
            lll::fmt::format("{:%Y-%m-%d %H:%M:%S}", t1));
  EXPECT_EQ(strftime_full_local(t1), lll::fmt::format("{}", t1));
  EXPECT_EQ(strftime_full_local(t1), lll::fmt::format("{:}", t1));
  using time_point = std::chrono::local_time<std::chrono::seconds>;
  auto t2 = time_point(std::chrono::seconds(86400 + 42));
  EXPECT_EQ(strftime_full_local(t2),
            lll::fmt::format("{:%Y-%m-%d %H:%M:%S}", t2));

  std::vector<std::string> spec_list = {
      "%%",  "%n",  "%t",  "%Y",  "%EY", "%y",  "%Oy", "%Ey", "%C",
      "%EC", "%G",  "%g",  "%b",  "%h",  "%B",  "%m",  "%Om", "%U",
      "%OU", "%W",  "%OW", "%V",  "%OV", "%j",  "%d",  "%Od", "%e",
      "%Oe", "%a",  "%A",  "%w",  "%Ow", "%u",  "%Ou", "%H",  "%OH",
      "%I",  "%OI", "%M",  "%OM", "%S",  "%OS", "%x",  "%Ex", "%X",
      "%EX", "%D",  "%F",  "%R",  "%T",  "%p",  "%z",  "%Z"};
#  ifndef _WIN32
  // Disabled on Windows because these formats are not consistent among
  // platforms.
  spec_list.insert(spec_list.end(), {"%c", "%Ec", "%r"});
#  elif !LAWS3_FMT_HAS_C99_STRFTIME
  // Only C89 conversion specifiers when using MSVCRT instead of UCRT
  spec_list = {"%%", "%Y", "%y", "%b", "%B", "%m", "%U", "%W", "%j", "%d", "%a",
               "%A", "%w", "%H", "%I", "%M", "%S", "%x", "%X", "%p", "%Z"};
#  endif
  spec_list.push_back("%Y-%m-%d %H:%M:%S");

  for (const auto& spec : spec_list) {
    auto t = std::chrono::system_clock::to_time_t(
        std::chrono::current_zone()->to_sys(t1));
    auto tm = *std::localtime(&t);

    auto sys_output = system_strftime(spec, &tm);

    auto fmt_spec = lll::fmt::format("{{:{}}}", spec);
    EXPECT_EQ(sys_output, lll::fmt::format(lll::fmt::runtime(fmt_spec), t1));
    EXPECT_EQ(sys_output, lll::fmt::format(lll::fmt::runtime(fmt_spec), tm));
  }

  if (std::find(spec_list.cbegin(), spec_list.cend(), "%z") !=
      spec_list.cend()) {
    auto t = std::chrono::system_clock::to_time_t(
        std::chrono::current_zone()->to_sys(t1));
    auto tm = *std::localtime(&t);

    auto sys_output = system_strftime("%z", &tm);
    sys_output.insert(sys_output.end() - 2, 1, ':');

    EXPECT_EQ(sys_output, lll::fmt::format("{:%Ez}", t1));
    EXPECT_EQ(sys_output, lll::fmt::format("{:%Ez}", tm));

    EXPECT_EQ(sys_output, lll::fmt::format("{:%Oz}", t1));
    EXPECT_EQ(sys_output, lll::fmt::format("{:%Oz}", tm));
  }
}

#endif  // LAWS3_FMT_USE_LOCAL_TIME

#ifndef LAWS3_FMT_STATIC_THOUSANDS_SEPARATOR

TEST(chrono_test, format_default) {
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::seconds(42)), "42s");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::atto>(42)),
            "42as");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::femto>(42)),
            "42fs");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::pico>(42)),
            "42ps");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::nanoseconds(42)), "42ns");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::microseconds(42)), "42µs");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::milliseconds(42)), "42ms");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::centi>(42)),
            "42cs");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::deci>(42)),
            "42ds");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::seconds(42)), "42s");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::deca>(42)),
            "42das");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::hecto>(42)),
            "42hs");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::kilo>(42)),
            "42ks");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::mega>(42)),
            "42Ms");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::giga>(42)),
            "42Gs");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::tera>(42)),
            "42Ts");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::peta>(42)),
            "42Ps");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<int, std::exa>(42)),
            "42Es");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::minutes(42)), "42min");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::hours(42)), "42h");
  EXPECT_EQ(lll::fmt::format("{}", days(42)), "42d");
  EXPECT_EQ(
      lll::fmt::format("{}", std::chrono::duration<int, std::ratio<15, 1>>(42)),
      "42[15]s");
  EXPECT_EQ(
      lll::fmt::format("{}", std::chrono::duration<int, std::ratio<15, 4>>(42)),
      "42[15/4]s");
}

TEST(chrono_test, duration_align) {
  auto s = std::chrono::seconds(42);
  EXPECT_EQ(lll::fmt::format("{:5}", s), "42s  ");
  EXPECT_EQ(lll::fmt::format("{:{}}", s, 5), "42s  ");
  EXPECT_EQ(lll::fmt::format("{:>5}", s), "  42s");
  EXPECT_EQ(lll::fmt::format("{:*^7}", s), "**42s**");
  EXPECT_EQ(lll::fmt::format("{:12%H:%M:%S}", std::chrono::seconds(12345)),
            "03:25:45    ");
  EXPECT_EQ(lll::fmt::format("{:>12%H:%M:%S}", std::chrono::seconds(12345)),
            "    03:25:45");
  EXPECT_EQ(lll::fmt::format("{:~^12%H:%M:%S}", std::chrono::seconds(12345)),
            "~~03:25:45~~");
  EXPECT_EQ(lll::fmt::format("{:{}%H:%M:%S}", std::chrono::seconds(12345), 12),
            "03:25:45    ");
}

TEST(chrono_test, tm_align) {
  auto t = make_tm(1975, 12, 29, 12, 14, 16);
  EXPECT_EQ(lll::fmt::format("{:%F %T}", t), "1975-12-29 12:14:16");
  EXPECT_EQ(lll::fmt::format("{:30%F %T}", t),
            "1975-12-29 12:14:16           ");
  EXPECT_EQ(lll::fmt::format("{:{}%F %T}", t, 30),
            "1975-12-29 12:14:16           ");
  EXPECT_EQ(lll::fmt::format("{:<30%F %T}", t),
            "1975-12-29 12:14:16           ");
  EXPECT_EQ(lll::fmt::format("{:^30%F %T}", t),
            "     1975-12-29 12:14:16      ");
  EXPECT_EQ(lll::fmt::format("{:>30%F %T}", t),
            "           1975-12-29 12:14:16");

  EXPECT_EQ(lll::fmt::format("{:*<30%F %T}", t),
            "1975-12-29 12:14:16***********");
  EXPECT_EQ(lll::fmt::format("{:*^30%F %T}", t),
            "*****1975-12-29 12:14:16******");
  EXPECT_EQ(lll::fmt::format("{:*>30%F %T}", t),
            "***********1975-12-29 12:14:16");
}

TEST(chrono_test, tp_align) {
  auto tp = std::chrono::time_point_cast<std::chrono::microseconds>(
      std::chrono::system_clock::from_time_t(0));
  EXPECT_EQ(lll::fmt::format("{:%M:%S}", tp), "00:00.000000");
  EXPECT_EQ(lll::fmt::format("{:15%M:%S}", tp), "00:00.000000   ");
  EXPECT_EQ(lll::fmt::format("{:{}%M:%S}", tp, 15), "00:00.000000   ");
  EXPECT_EQ(lll::fmt::format("{:<15%M:%S}", tp), "00:00.000000   ");
  EXPECT_EQ(lll::fmt::format("{:^15%M:%S}", tp), " 00:00.000000  ");
  EXPECT_EQ(lll::fmt::format("{:>15%M:%S}", tp), "   00:00.000000");

  EXPECT_EQ(lll::fmt::format("{:*<15%M:%S}", tp), "00:00.000000***");
  EXPECT_EQ(lll::fmt::format("{:*^15%M:%S}", tp), "*00:00.000000**");
  EXPECT_EQ(lll::fmt::format("{:*>15%M:%S}", tp), "***00:00.000000");
}

TEST(chrono_test, format_specs) {
  EXPECT_EQ(lll::fmt::format("{:%%}", std::chrono::seconds(0)), "%");
  EXPECT_EQ(lll::fmt::format("{:%n}", std::chrono::seconds(0)), "\n");
  EXPECT_EQ(lll::fmt::format("{:%t}", std::chrono::seconds(0)), "\t");
  EXPECT_EQ(lll::fmt::format("{:%S}", std::chrono::seconds(0)), "00");
  EXPECT_EQ(lll::fmt::format("{:%S}", std::chrono::seconds(60)), "00");
  EXPECT_EQ(lll::fmt::format("{:%S}", std::chrono::seconds(42)), "42");
  EXPECT_EQ(lll::fmt::format("{:%S}", std::chrono::milliseconds(1234)),
            "01.234");
  EXPECT_EQ(lll::fmt::format("{:%M}", std::chrono::minutes(0)), "00");
  EXPECT_EQ(lll::fmt::format("{:%M}", std::chrono::minutes(60)), "00");
  EXPECT_EQ(lll::fmt::format("{:%M}", std::chrono::minutes(42)), "42");
  EXPECT_EQ(lll::fmt::format("{:%M}", std::chrono::seconds(61)), "01");
  EXPECT_EQ(lll::fmt::format("{:%H}", std::chrono::hours(0)), "00");
  EXPECT_EQ(lll::fmt::format("{:%H}", std::chrono::hours(24)), "00");
  EXPECT_EQ(lll::fmt::format("{:%H}", std::chrono::hours(14)), "14");
  EXPECT_EQ(lll::fmt::format("{:%H}", std::chrono::minutes(61)), "01");
  EXPECT_EQ(lll::fmt::format("{:%I}", std::chrono::hours(0)), "12");
  EXPECT_EQ(lll::fmt::format("{:%I}", std::chrono::hours(12)), "12");
  EXPECT_EQ(lll::fmt::format("{:%I}", std::chrono::hours(24)), "12");
  EXPECT_EQ(lll::fmt::format("{:%I}", std::chrono::hours(4)), "04");
  EXPECT_EQ(lll::fmt::format("{:%I}", std::chrono::hours(14)), "02");
  EXPECT_EQ(lll::fmt::format("{:%j}", days(12345)), "12345");
  EXPECT_EQ(lll::fmt::format("{:%j}", std::chrono::hours(12345 * 24 + 12)),
            "12345");
  EXPECT_EQ(lll::fmt::format("{:%H:%M:%S}", std::chrono::seconds(12345)),
            "03:25:45");
  EXPECT_EQ(lll::fmt::format("{:%R}", std::chrono::seconds(12345)), "03:25");
  EXPECT_EQ(lll::fmt::format("{:%T}", std::chrono::seconds(12345)), "03:25:45");
  EXPECT_EQ(lll::fmt::format("{:%Q}", std::chrono::seconds(12345)), "12345");
  EXPECT_EQ(lll::fmt::format("{:%q}", std::chrono::seconds(12345)), "s");
}

TEST(chrono_test, invalid_specs) {
  auto sec = std::chrono::seconds(0);
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%a}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%A}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%c}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%x}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%Ex}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%X}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%EX}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%D}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%F}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%Ec}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%w}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%u}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%b}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%B}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%z}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%Z}"), sec),
                   lll::fmt::format_error, "no date");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%Eq}"), sec),
                   lll::fmt::format_error, "invalid format");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:%Oq}"), sec),
                   lll::fmt::format_error, "invalid format");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:abc}"), sec),
                   lll::fmt::format_error, "invalid format");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{:.2f}"), sec),
                   lll::fmt::format_error, "invalid format");
}

auto format_tm(const std::tm& time, lll::fmt::string_view spec,
               const std::locale& loc) -> std::string {
  auto& facet = std::use_facet<std::time_put<char>>(loc);
  std::ostringstream os;
  os.imbue(loc);
  facet.put(os, os, ' ', &time, spec.begin(), spec.end());
  return os.str();
}

TEST(chrono_test, locale) {
  auto loc = get_locale("ja_JP.utf8");
  if (loc == std::locale::classic()) return;
#  define EXPECT_TIME(spec, time, duration)                          \
    {                                                                \
      auto jp_loc = std::locale("ja_JP.utf8");                       \
      EXPECT_EQ(format_tm(time, spec, jp_loc),                       \
                lll::fmt::format(jp_loc, "{:L" spec "}", duration)); \
    }
  EXPECT_TIME("%OH", make_hour(14), std::chrono::hours(14));
  EXPECT_TIME("%OI", make_hour(14), std::chrono::hours(14));
  EXPECT_TIME("%OM", make_minute(42), std::chrono::minutes(42));
  EXPECT_TIME("%OS", make_second(42), std::chrono::seconds(42));
  auto time = make_tm();
  time.tm_hour = 3;
  time.tm_min = 25;
  time.tm_sec = 45;
  auto sec = std::chrono::seconds(12345);
  EXPECT_TIME("%r", time, sec);
  EXPECT_TIME("%p", time, sec);
}

using dms = std::chrono::duration<double, std::milli>;

TEST(chrono_test, format_default_fp) {
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<float>(1.234)),
            "1.234s");
  EXPECT_EQ(
      lll::fmt::format("{}", std::chrono::duration<float, std::milli>(1.234)),
      "1.234ms");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<double>(1.234)),
            "1.234s");
  EXPECT_EQ(lll::fmt::format("{}", dms(1.234)), "1.234ms");
}

TEST(chrono_test, format_precision) {
  EXPECT_THROW_MSG(
      (void)lll::fmt::format(runtime("{:.2%Q}"), std::chrono::seconds(42)),
      lll::fmt::format_error, "precision not allowed for this argument type");
  EXPECT_EQ(lll::fmt::format("{:.0}", dms(1.234)), "1ms");
  EXPECT_EQ(lll::fmt::format("{:.1}", dms(1.234)), "1.2ms");
  EXPECT_EQ(lll::fmt::format("{:.{}}", dms(1.234), 2), "1.23ms");

  EXPECT_EQ(lll::fmt::format("{:.0}", dms(12.56)), "13ms");
  EXPECT_EQ(lll::fmt::format("{:.1}", dms(12.56)), "12.6ms");
  EXPECT_EQ(lll::fmt::format("{:.2}", dms(12.56)), "12.56ms");
}

TEST(chrono_test, format_full_specs) {
  EXPECT_EQ(lll::fmt::format("{:6.0}", dms(1.234)), "1ms   ");
  EXPECT_EQ(lll::fmt::format("{:6.1}", dms(1.234)), "1.2ms ");
  EXPECT_EQ(lll::fmt::format("{:>8.{}}", dms(1.234), 2), "  1.23ms");
  EXPECT_EQ(lll::fmt::format("{:^{}.{}}", dms(1.234), 7, 1), " 1.2ms ");
  EXPECT_EQ(lll::fmt::format("{0:^{2}.{1}}", dms(1.234), 2, 8), " 1.23ms ");
  EXPECT_EQ(lll::fmt::format("{:=^{}.{}}", dms(1.234), 9, 3), "=1.234ms=");
  EXPECT_EQ(lll::fmt::format("{:*^10.4}", dms(1.234)), "*1.2340ms*");

  EXPECT_EQ(lll::fmt::format("{:6.0}", dms(12.56)), "13ms  ");
  EXPECT_EQ(lll::fmt::format("{:>8.{}}", dms(12.56), 0), "    13ms");
  EXPECT_EQ(lll::fmt::format("{:^{}.{}}", dms(12.56), 6, 0), " 13ms ");
  EXPECT_EQ(lll::fmt::format("{0:^{2}.{1}}", dms(12.56), 0, 8), "  13ms  ");
  EXPECT_EQ(lll::fmt::format("{:=^{}.{}}", dms(12.56), 9, 0), "==13ms===");
  EXPECT_EQ(lll::fmt::format("{:*^10.0}", dms(12.56)), "***13ms***");
}

TEST(chrono_test, format_simple_q) {
  EXPECT_EQ(lll::fmt::format("{:%Q %q}", std::chrono::duration<float>(1.234)),
            "1.234 s");
  EXPECT_EQ(lll::fmt::format("{:%Q %q}",
                             std::chrono::duration<float, std::milli>(1.234)),
            "1.234 ms");
  EXPECT_EQ(lll::fmt::format("{:%Q %q}", std::chrono::duration<double>(1.234)),
            "1.234 s");
  EXPECT_EQ(lll::fmt::format("{:%Q %q}", dms(1.234)), "1.234 ms");
}

TEST(chrono_test, format_precision_q) {
  EXPECT_THROW_MSG(
      (void)lll::fmt::format(runtime("{:.2%Q %q}"), std::chrono::seconds(42)),
      lll::fmt::format_error, "precision not allowed for this argument type");
  EXPECT_EQ(lll::fmt::format("{:.1%Q %q}", dms(1.234)), "1.2 ms");
  EXPECT_EQ(lll::fmt::format("{:.{}%Q %q}", dms(1.234), 2), "1.23 ms");
}

TEST(chrono_test, format_full_specs_q) {
  EXPECT_EQ(lll::fmt::format("{:7.0%Q %q}", dms(1.234)), "1 ms   ");
  EXPECT_EQ(lll::fmt::format("{:7.1%Q %q}", dms(1.234)), "1.2 ms ");
  EXPECT_EQ(lll::fmt::format("{:>8.{}%Q %q}", dms(1.234), 2), " 1.23 ms");
  EXPECT_EQ(lll::fmt::format("{:^{}.{}%Q %q}", dms(1.234), 8, 1), " 1.2 ms ");
  EXPECT_EQ(lll::fmt::format("{0:^{2}.{1}%Q %q}", dms(1.234), 2, 9),
            " 1.23 ms ");
  EXPECT_EQ(lll::fmt::format("{:=^{}.{}%Q %q}", dms(1.234), 10, 3),
            "=1.234 ms=");
  EXPECT_EQ(lll::fmt::format("{:*^11.4%Q %q}", dms(1.234)), "*1.2340 ms*");

  EXPECT_EQ(lll::fmt::format("{:7.0%Q %q}", dms(12.56)), "13 ms  ");
  EXPECT_EQ(lll::fmt::format("{:>8.{}%Q %q}", dms(12.56), 0), "   13 ms");
  EXPECT_EQ(lll::fmt::format("{:^{}.{}%Q %q}", dms(12.56), 8, 0), " 13 ms  ");
  EXPECT_EQ(lll::fmt::format("{0:^{2}.{1}%Q %q}", dms(12.56), 0, 9),
            "  13 ms  ");
  EXPECT_EQ(lll::fmt::format("{:=^{}.{}%Q %q}", dms(12.56), 9, 0), "==13 ms==");
  EXPECT_EQ(lll::fmt::format("{:*^11.0%Q %q}", dms(12.56)), "***13 ms***");
}

TEST(chrono_test, invalid_width_id) {
  EXPECT_THROW(
      (void)lll::fmt::format(runtime("{:{o}"), std::chrono::seconds(0)),
      lll::fmt::format_error);
}

TEST(chrono_test, invalid_colons) {
  EXPECT_THROW(
      (void)lll::fmt::format(runtime("{0}=:{0::"), std::chrono::seconds(0)),
      lll::fmt::format_error);
}

TEST(chrono_test, negative_durations) {
  EXPECT_EQ(lll::fmt::format("{:%Q}", std::chrono::seconds(-12345)), "-12345");
  EXPECT_EQ(lll::fmt::format("{:%H:%M:%S}", std::chrono::seconds(-12345)),
            "-03:25:45");
  EXPECT_EQ(lll::fmt::format("{:%M:%S}", std::chrono::duration<double>(-1)),
            "-00:01");
  EXPECT_EQ(lll::fmt::format("{:%q}", std::chrono::seconds(-12345)), "s");
  EXPECT_EQ(lll::fmt::format(
                "{:%S}", std::chrono::duration<signed char, std::milli>(-127)),
            "-00.127");
  auto min = std::numeric_limits<int>::min();
  EXPECT_EQ(lll::fmt::format("{}", min),
            lll::fmt::format("{:%Q}", std::chrono::duration<int>(min)));
}

TEST(chrono_test, special_durations) {
  EXPECT_EQ(lll::fmt::format("{:%S}", std::chrono::duration<double>(1e20)),
            "40");
  auto nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(lll::fmt::format("{:%I %H %M %S %R %r}",
                             std::chrono::duration<double>(nan)),
            "nan nan nan nan nan:nan nan");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<float, std::exa>(1)),
            "1Es");
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<float, std::atto>(1)),
            "1as");
  EXPECT_EQ(
      lll::fmt::format("{:%R}", std::chrono::duration<char, std::mega>{2}),
      "03:33");
  EXPECT_EQ(
      lll::fmt::format("{:%T}", std::chrono::duration<char, std::mega>{2}),
      "03:33:20");
  EXPECT_EQ(lll::fmt::format("{:.3%S}",
                             std::chrono::duration<float, std::pico>(1.234e12)),
            "01.234");
}

TEST(chrono_test, unsigned_duration) {
  EXPECT_EQ(lll::fmt::format("{}", std::chrono::duration<unsigned>(42)), "42s");
}

TEST(chrono_test, weekday) {
  auto loc = get_locale("es_ES.UTF-8");
  std::locale::global(loc);
  auto sat = lll::fmt::weekday(6);

  auto tm = std::tm();
  tm.tm_wday = static_cast<int>(sat.c_encoding());

  EXPECT_EQ(lll::fmt::format("{}", sat), "Sat");
  EXPECT_EQ(lll::fmt::format("{:%a}", tm), "Sat");

  if (loc != std::locale::classic()) {
    auto saturdays = std::vector<std::string>{"sáb", "sá."};
    EXPECT_THAT(saturdays, Contains(lll::fmt::format(loc, "{:L}", sat)));
    EXPECT_THAT(saturdays, Contains(lll::fmt::format(loc, "{:%a}", tm)));
  }
}

TEST(chrono_test, cpp20_duration_subsecond_support) {
  using attoseconds = std::chrono::duration<long long, std::atto>;
  // Check that 18 digits of subsecond precision are supported.
  EXPECT_EQ(lll::fmt::format("{:%S}", attoseconds{999999999999999999}),
            "00.999999999999999999");
  EXPECT_EQ(lll::fmt::format("{:%S}", attoseconds{673231113420148734}),
            "00.673231113420148734");
  EXPECT_EQ(lll::fmt::format("{:%S}", attoseconds{-673231113420148734}),
            "-00.673231113420148734");
  EXPECT_EQ(lll::fmt::format("{:%S}", std::chrono::nanoseconds{13420148734}),
            "13.420148734");
  EXPECT_EQ(lll::fmt::format("{:%S}", std::chrono::nanoseconds{-13420148734}),
            "-13.420148734");
  EXPECT_EQ(lll::fmt::format("{:%S}", std::chrono::milliseconds{1234}),
            "01.234");
  // Check subsecond presision modifier.
  EXPECT_EQ(lll::fmt::format("{:.6%S}", std::chrono::nanoseconds{1234}),
            "00.000001");
  EXPECT_EQ(lll::fmt::format("{:.18%S}", std::chrono::nanoseconds{1234}),
            "00.000001234000000000");
  EXPECT_EQ(lll::fmt::format("{:.{}%S}", std::chrono::nanoseconds{1234}, 6),
            "00.000001");
  EXPECT_EQ(lll::fmt::format("{:.6%S}", std::chrono::milliseconds{1234}),
            "01.234000");
  EXPECT_EQ(lll::fmt::format("{:.6%S}", std::chrono::milliseconds{-1234}),
            "-01.234000");
  EXPECT_EQ(lll::fmt::format("{:.3%S}", std::chrono::seconds{1234}), "34.000");
  EXPECT_EQ(lll::fmt::format("{:.3%S}", std::chrono::hours{1234}), "00.000");
  EXPECT_EQ(lll::fmt::format("{:.5%S}", dms(1.234)), "00.00123");
  EXPECT_EQ(lll::fmt::format("{:.8%S}", dms(1.234)), "00.00123400");
  {
    // Check that {:%H:%M:%S} is equivalent to {:%T}.
    auto dur = std::chrono::milliseconds{3601234};
    auto formatted_dur = lll::fmt::format("{:%T}", dur);
    EXPECT_EQ(formatted_dur, "01:00:01.234");
    EXPECT_EQ(lll::fmt::format("{:%H:%M:%S}", dur), formatted_dur);
    EXPECT_EQ(lll::fmt::format("{:.6%H:%M:%S}", dur), "01:00:01.234000");
  }
  using nanoseconds_dbl = std::chrono::duration<double, std::nano>;
  EXPECT_EQ(lll::fmt::format("{:%S}", nanoseconds_dbl{-123456789}),
            "-00.123456789");
  EXPECT_EQ(lll::fmt::format("{:%S}", nanoseconds_dbl{9123456789}),
            "09.123456789");
  // Verify that only the seconds part is extracted and printed.
  EXPECT_EQ(lll::fmt::format("{:%S}", nanoseconds_dbl{99123456789}),
            "39.123456789");
  EXPECT_EQ(lll::fmt::format("{:%S}", nanoseconds_dbl{99123000000}),
            "39.123000000");
  {
    // Now the hour is printed, and we also test if negative doubles work.
    auto dur = nanoseconds_dbl{-99123456789};
    auto formatted_dur = lll::fmt::format("{:%T}", dur);
    EXPECT_EQ(formatted_dur, "-00:01:39.123456789");
    EXPECT_EQ(lll::fmt::format("{:%H:%M:%S}", dur), formatted_dur);
    EXPECT_EQ(lll::fmt::format("{:.3%H:%M:%S}", dur), "-00:01:39.123");
  }
  // Check that durations with precision greater than std::chrono::seconds have
  // fixed precision, and print zeros even if there is no fractional part.
  EXPECT_EQ(lll::fmt::format("{:%S}", std::chrono::microseconds{7000000}),
            "07.000000");
  EXPECT_EQ(lll::fmt::format(
                "{:%S}", std::chrono::duration<long long, std::ratio<1, 3>>(1)),
            "00.333333");
  EXPECT_EQ(lll::fmt::format(
                "{:%S}", std::chrono::duration<long long, std::ratio<1, 7>>(1)),
            "00.142857");

  EXPECT_EQ(lll::fmt::format(
                "{:%S}",
                std::chrono::duration<signed char, std::ratio<1, 100>>(0x80)),
            "-01.28");

  EXPECT_EQ(
      lll::fmt::format(
          "{:%M:%S}", std::chrono::duration<short, std::ratio<1, 100>>(0x8000)),
      "-05:27.68");

  // Check that floating point seconds with ratio<1,1> are printed.
  EXPECT_EQ(lll::fmt::format("{:%S}", std::chrono::duration<double>{1.5}),
            "01.500000");
  EXPECT_EQ(lll::fmt::format("{:%M:%S}", std::chrono::duration<double>{-61.25}),
            "-01:01.250000");
}

#endif  // LAWS3_FMT_STATIC_THOUSANDS_SEPARATOR

// Disable the utc_clock test for windows, as the icu.dll used for tzdb
// (time zone database) is not shipped with many windows versions.
#if LAWS3_FMT_USE_UTC_TIME && !defined(_WIN32)
TEST(chrono_test, utc_clock) {
  auto t1 = std::chrono::system_clock::now();
  auto t1_utc = std::chrono::utc_clock::from_sys(t1);
  EXPECT_EQ(lll::fmt::format("{:%Y-%m-%d %H:%M:%S}", t1),
            lll::fmt::format("{:%Y-%m-%d %H:%M:%S}", t1_utc));
}
#endif

TEST(chrono_test, timestamp_ratios) {
  auto t1 =
      sys_time<std::chrono::milliseconds>(std::chrono::milliseconds(67890));
  EXPECT_EQ(lll::fmt::format("{:%M:%S}", t1), "01:07.890");

  auto t2 = sys_time<std::chrono::minutes>(std::chrono::minutes(7));
  EXPECT_EQ(lll::fmt::format("{:%M:%S}", t2), "07:00");

  auto t3 = sys_time<std::chrono::duration<int, std::ratio<9>>>(
      std::chrono::duration<int, std::ratio<9>>(7));
  EXPECT_EQ(lll::fmt::format("{:%M:%S}", t3), "01:03");

  auto t4 = sys_time<std::chrono::duration<int, std::ratio<63>>>(
      std::chrono::duration<int, std::ratio<63>>(1));
  EXPECT_EQ(lll::fmt::format("{:%M:%S}", t4), "01:03");

  if (sizeof(time_t) > 4) {
    auto tp =
        sys_time<std::chrono::milliseconds>(std::chrono::seconds(32503680000));
    EXPECT_EQ(lll::fmt::format("{:%Y-%m-%d}", tp), "3000-01-01");
  }

  if (LAWS3_FMT_SAFE_DURATION_CAST) {
    using years = std::chrono::duration<std::int64_t, std::ratio<31556952>>;
    auto tp = sys_time<years>(years(std::numeric_limits<std::int64_t>::max()));
    EXPECT_THROW_MSG((void)lll::fmt::format("{:%Y-%m-%d}", tp),
                     lll::fmt::format_error, "cannot format duration");
  }
}

TEST(chrono_test, timestamp_sub_seconds) {
  auto t1 = sys_time<std::chrono::duration<long long, std::ratio<1, 3>>>(
      std::chrono::duration<long long, std::ratio<1, 3>>(4));
  EXPECT_EQ(lll::fmt::format("{:%S}", t1), "01.333333");

  auto t2 = sys_time<std::chrono::duration<double, std::ratio<1, 3>>>(
      std::chrono::duration<double, std::ratio<1, 3>>(4));
  EXPECT_EQ(lll::fmt::format("{:%S}", t2), "01.333333");

  auto t3 = sys_time<std::chrono::seconds>(std::chrono::seconds(2));
  EXPECT_EQ(lll::fmt::format("{:%S}", t3), "02");

  auto t4 = sys_time<std::chrono::duration<double>>(
      std::chrono::duration<double, std::ratio<1, 1>>(9.5));
  EXPECT_EQ(lll::fmt::format("{:%S}", t4), "09.500000");

  auto t5 = sys_time<std::chrono::duration<double>>(
      std::chrono::duration<double, std::ratio<1, 1>>(9));
  EXPECT_EQ(lll::fmt::format("{:%S}", t5), "09");

  auto t6 = sys_time<std::chrono::milliseconds>(std::chrono::seconds(1) +
                                                std::chrono::milliseconds(120));
  EXPECT_EQ(lll::fmt::format("{:%S}", t6), "01.120");

  auto t7 =
      sys_time<std::chrono::microseconds>(std::chrono::microseconds(1234567));
  EXPECT_EQ(lll::fmt::format("{:%S}", t7), "01.234567");

  auto t8 =
      sys_time<std::chrono::nanoseconds>(std::chrono::nanoseconds(123456789));
  EXPECT_EQ(lll::fmt::format("{:%S}", t8), "00.123456789");

  auto t9 = std::chrono::time_point_cast<std::chrono::nanoseconds>(
      std::chrono::system_clock::now());
  auto t9_sec = std::chrono::time_point_cast<std::chrono::seconds>(t9);
  auto t9_sub_sec_part = lll::fmt::format("{0:09}", (t9 - t9_sec).count());
  EXPECT_EQ(
      lll::fmt::format("{}.{}", strftime_full_utc(t9_sec), t9_sub_sec_part),
      lll::fmt::format("{:%Y-%m-%d %H:%M:%S}", t9));
  EXPECT_EQ(
      lll::fmt::format("{}.{}", strftime_full_utc(t9_sec), t9_sub_sec_part),
      lll::fmt::format("{:%Y-%m-%d %T}", t9));

  auto t10 =
      sys_time<std::chrono::milliseconds>(std::chrono::milliseconds(2000));
  EXPECT_EQ(lll::fmt::format("{:%S}", t10), "02.000");

  auto epoch = sys_time<std::chrono::milliseconds>();
  auto d = std::chrono::milliseconds(250);
  EXPECT_EQ(lll::fmt::format("{:%S}", epoch - d), "59.750");
  EXPECT_EQ(lll::fmt::format("{:%S}", epoch), "00.000");
  EXPECT_EQ(lll::fmt::format("{:%S}", epoch + d), "00.250");
}

TEST(chrono_test, glibc_extensions) {
  EXPECT_THROW_MSG(
      (void)lll::fmt::format(runtime("{:%0}"), std::chrono::seconds()),
      lll::fmt::format_error, "invalid format");
  EXPECT_THROW_MSG(
      (void)lll::fmt::format(runtime("{:%_}"), std::chrono::seconds()),
      lll::fmt::format_error, "invalid format");
  EXPECT_THROW_MSG(
      (void)lll::fmt::format(runtime("{:%-}"), std::chrono::seconds()),
      lll::fmt::format_error, "invalid format");

  {
    const auto d = std::chrono::hours(1) + std::chrono::minutes(2) +
                   std::chrono::seconds(3);

    EXPECT_EQ(lll::fmt::format("{:%I,%H,%M,%S}", d), "01,01,02,03");
    EXPECT_EQ(lll::fmt::format("{:%0I,%0H,%0M,%0S}", d), "01,01,02,03");
    EXPECT_EQ(lll::fmt::format("{:%_I,%_H,%_M,%_S}", d), " 1, 1, 2, 3");
    EXPECT_EQ(lll::fmt::format("{:%-I,%-H,%-M,%-S}", d), "1,1,2,3");
    EXPECT_EQ(lll::fmt::format("{:%-I,%H,%M,%S}", d), "1,01,02,03");

    EXPECT_EQ(lll::fmt::format("{:%OI,%OH,%OM,%OS}", d), "01,01,02,03");
    EXPECT_EQ(lll::fmt::format("{:%0OI,%0OH,%0OM,%0OS}", d), "01,01,02,03");
    EXPECT_EQ(lll::fmt::format("{:%_OI,%_OH,%_OM,%_OS}", d), " 1, 1, 2, 3");
    EXPECT_EQ(lll::fmt::format("{:%-OI,%-OH,%-OM,%-OS}", d), "1,1,2,3");
  }

  {
    const auto tm = make_tm(1970, 1, 1, 1, 2, 3);
    EXPECT_EQ(lll::fmt::format("{:%I,%H,%M,%S}", tm), "01,01,02,03");
    EXPECT_EQ(lll::fmt::format("{:%0I,%0H,%0M,%0S}", tm), "01,01,02,03");
    EXPECT_EQ(lll::fmt::format("{:%_I,%_H,%_M,%_S}", tm), " 1, 1, 2, 3");
    EXPECT_EQ(lll::fmt::format("{:%-I,%-H,%-M,%-S}", tm), "1,1,2,3");

    EXPECT_EQ(lll::fmt::format("{:%OI,%OH,%OM,%OS}", tm), "01,01,02,03");
    EXPECT_EQ(lll::fmt::format("{:%0OI,%0OH,%0OM,%0OS}", tm), "01,01,02,03");
    EXPECT_EQ(lll::fmt::format("{:%_OI,%_OH,%_OM,%_OS}", tm), " 1, 1, 2, 3");
    EXPECT_EQ(lll::fmt::format("{:%-OI,%-OH,%-OM,%-OS}", tm), "1,1,2,3");
  }

  {
    const auto d = std::chrono::seconds(3) + std::chrono::milliseconds(140);
    EXPECT_EQ(lll::fmt::format("{:%S}", d), "03.140");
    EXPECT_EQ(lll::fmt::format("{:%0S}", d), "03.140");
    EXPECT_EQ(lll::fmt::format("{:%_S}", d), " 3.140");
    EXPECT_EQ(lll::fmt::format("{:%-S}", d), "3.140");
  }

  {
    const auto d = std::chrono::duration<double>(3.14);
    EXPECT_EQ(lll::fmt::format("{:%S}", d), "03.140000");
    EXPECT_EQ(lll::fmt::format("{:%0S}", d), "03.140000");
    EXPECT_EQ(lll::fmt::format("{:%_S}", d), " 3.140000");
    EXPECT_EQ(lll::fmt::format("{:%-S}", d), "3.140000");
  }
}

TEST(chrono_test, out_of_range) {
  auto d = std::chrono::duration<unsigned long, std::giga>(538976288);
  EXPECT_THROW((void)lll::fmt::format("{:%j}", d), lll::fmt::format_error);
}
