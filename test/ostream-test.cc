// Formatting library for C++ - std::ostream support tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <fstream>

#include "3laws/fmt/format.h"

using lll::fmt::runtime;

struct test {};

// Test that there is no issues with specializations when fmt/ostream.h is
// included after fmt/format.h.
namespace lll {
namespace fmt {
template <> struct formatter<test> : formatter<int> {
  auto format(const test&, format_context& ctx) const -> decltype(ctx.out()) {
    return formatter<int>::format(42, ctx);
  }
};
}  // namespace fmt
}  // namespace lll

#include <sstream>

#include "3laws/fmt/compile.h"
#include "3laws/fmt/ostream.h"
#include "3laws/fmt/ranges.h"
#include "gmock/gmock.h"
#include "gtest-extra.h"
#include "util.h"

auto operator<<(std::ostream& os, const date& d) -> std::ostream& {
  os << d.year() << '-' << d.month() << '-' << d.day();
  return os;
}

auto operator<<(std::wostream& os, const date& d) -> std::wostream& {
  os << d.year() << L'-' << d.month() << L'-' << d.day();
  return os;
}

// Make sure that overloaded comma operators do no harm to is_streamable.
struct type_with_comma_op {};
template <typename T> void operator,(type_with_comma_op, const T&);
template <typename T> type_with_comma_op operator<<(T&, const date&);

enum streamable_enum {};

auto operator<<(std::ostream& os, streamable_enum) -> std::ostream& {
  return os << "streamable_enum";
}

enum unstreamable_enum {};
auto format_as(unstreamable_enum e) -> int { return e; }

struct empty_test {};
auto operator<<(std::ostream& os, empty_test) -> std::ostream& {
  return os << "";
}

namespace lll {
namespace fmt {
template <> struct formatter<test_string> : ostream_formatter {};
template <> struct formatter<date> : ostream_formatter {};
template <> struct formatter<streamable_enum> : ostream_formatter {};
template <> struct formatter<empty_test> : ostream_formatter {};
}  // namespace fmt
}  // namespace lll

TEST(ostream_test, enum) {
  EXPECT_EQ("streamable_enum", lll::fmt::format("{}", streamable_enum()));
  EXPECT_EQ("0", lll::fmt::format("{}", unstreamable_enum()));
}

TEST(ostream_test, format) {
  EXPECT_EQ("a string", lll::fmt::format("{0}", test_string("a string")));
  EXPECT_EQ("The date is 2012-12-9",
            lll::fmt::format("The date is {0}", date(2012, 12, 9)));
}

TEST(ostream_test, format_specs) {
  using lll::fmt::format_error;
  EXPECT_EQ("def  ", lll::fmt::format("{0:<5}", test_string("def")));
  EXPECT_EQ("  def", lll::fmt::format("{0:>5}", test_string("def")));
  EXPECT_EQ(" def ", lll::fmt::format("{0:^5}", test_string("def")));
  EXPECT_EQ("def**", lll::fmt::format("{0:*<5}", test_string("def")));
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{0:+}"), test_string()),
                   format_error, "invalid format specifier");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{0:-}"), test_string()),
                   format_error, "invalid format specifier");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{0: }"), test_string()),
                   format_error, "invalid format specifier");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{0:#}"), test_string()),
                   format_error, "invalid format specifier");
  EXPECT_THROW_MSG((void)lll::fmt::format(runtime("{0:05}"), test_string()),
                   format_error, "format specifier requires numeric argument");
  EXPECT_EQ("test         ", lll::fmt::format("{0:13}", test_string("test")));
  EXPECT_EQ("test         ",
            lll::fmt::format("{0:{1}}", test_string("test"), 13));
  EXPECT_EQ("te", lll::fmt::format("{0:.2}", test_string("test")));
  EXPECT_EQ("te", lll::fmt::format("{0:.{1}}", test_string("test"), 2));
}

TEST(ostream_test, empty_custom_output) {
  EXPECT_EQ("", lll::fmt::format("{}", empty_test()));
}

TEST(ostream_test, print) {
  {
    std::ostringstream os;
    lll::fmt::print(os, "Don't {}!", "panic");
    EXPECT_EQ("Don't panic!", os.str());
  }

  {
    std::ostringstream os;
    lll::fmt::println(os, "Don't {}!", "panic");
    EXPECT_EQ("Don't panic!\n", os.str());
  }
}

TEST(ostream_test, write_to_ostream) {
  std::ostringstream os;
  lll::fmt::memory_buffer buffer;
  const char* foo = "foo";
  buffer.append(foo, foo + std::strlen(foo));
  lll::fmt::detail::write_buffer(os, buffer);
  EXPECT_EQ("foo", os.str());
}

TEST(ostream_test, write_to_ostream_max_size) {
  auto max_size = lll::fmt::detail::max_value<size_t>();
  auto max_streamsize = lll::fmt::detail::max_value<std::streamsize>();
  if (max_size <= lll::fmt::detail::to_unsigned(max_streamsize)) return;

  struct test_buffer final : lll::fmt::detail::buffer<char> {
    explicit test_buffer(size_t size)
        : lll::fmt::detail::buffer<char>([](buffer<char>&, size_t) {}, nullptr,
                                         size, size) {}
  } buffer(max_size);

  struct mock_streambuf : std::streambuf {
    MOCK_METHOD(std::streamsize, xsputn, (const void*, std::streamsize));
    auto xsputn(const char* s, std::streamsize n) -> std::streamsize override {
      const void* v = s;
      return xsputn(v, n);
    }
  } streambuf;

  struct test_ostream : std::ostream {
    explicit test_ostream(mock_streambuf& output_buffer)
        : std::ostream(&output_buffer) {}
  } os(streambuf);

  testing::InSequence sequence;
  const char* data = nullptr;
  using ustreamsize = std::make_unsigned<std::streamsize>::type;
  ustreamsize size = max_size;
  do {
    auto n = std::min(size, lll::fmt::detail::to_unsigned(max_streamsize));
    EXPECT_CALL(streambuf, xsputn(data, static_cast<std::streamsize>(n)))
        .WillOnce(testing::Return(max_streamsize));
    data += n;
    size -= n;
  } while (size != 0);
  lll::fmt::detail::write_buffer(os, buffer);
}

TEST(ostream_test, join) {
  int v[3] = {1, 2, 3};
  EXPECT_EQ("1, 2, 3", lll::fmt::format("{}", lll::fmt::join(v, v + 3, ", ")));
}

TEST(ostream_test, join_fallback_formatter) {
  auto strs = std::vector<test_string>{test_string("foo"), test_string("bar")};
  EXPECT_EQ("foo, bar", lll::fmt::format("{}", lll::fmt::join(strs, ", ")));
}

#if LAWS3_FMT_USE_CONSTEXPR
TEST(ostream_test, constexpr_string) {
  EXPECT_EQ("42", lll::fmt::format(LAWS3_FMT_STRING("{}"), std::string("42")));
  EXPECT_EQ("a string",
            lll::fmt::format(LAWS3_FMT_STRING("{0}"), test_string("a string")));
}
#endif

namespace fmt_test {
struct abc {};

template <typename Output> auto operator<<(Output& out, abc) -> Output& {
  return out << "abc";
}
}  // namespace fmt_test

template <typename T> struct test_template {};

template <typename T>
auto operator<<(std::ostream& os, test_template<T>) -> std::ostream& {
  return os << 1;
}

namespace lll {
namespace fmt {
template <typename T> struct formatter<test_template<T>> : formatter<int> {
  auto format(test_template<T>, format_context& ctx) const
      -> decltype(ctx.out()) {
    return formatter<int>::format(2, ctx);
  }
};

template <> struct formatter<fmt_test::abc> : ostream_formatter {};
}  // namespace fmt
}  // namespace lll

TEST(ostream_test, template) {
  EXPECT_EQ("2", lll::fmt::format("{}", test_template<int>()));
}

TEST(ostream_test, format_to_n) {
  char buffer[4];
  buffer[3] = 'x';
  auto result = lll::fmt::format_to_n(buffer, 3, "{}", fmt_test::abc());
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("abcx", lll::fmt::string_view(buffer, 4));
  result = lll::fmt::format_to_n(buffer, 3, "x{}y", fmt_test::abc());
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("xabx", lll::fmt::string_view(buffer, 4));
}

struct copyfmt_test {};

std::ostream& operator<<(std::ostream& os, copyfmt_test) {
  std::ios ios(nullptr);
  ios.copyfmt(os);
  return os << "foo";
}

namespace lll {
namespace fmt {
template <> struct formatter<copyfmt_test> : ostream_formatter {};
}  // namespace fmt
}  // namespace lll

TEST(ostream_test, copyfmt) {
  EXPECT_EQ("foo", lll::fmt::format("{}", copyfmt_test()));
}

TEST(ostream_test, to_string) {
  EXPECT_EQ("abc", lll::fmt::to_string(fmt_test::abc()));
}

TEST(ostream_test, range) {
  auto strs = std::vector<test_string>{test_string("foo"), test_string("bar")};
  EXPECT_EQ("[foo, bar]", lll::fmt::format("{}", strs));
}

struct abstract {
  virtual ~abstract() = default;
  virtual void f() = 0;
  friend auto operator<<(std::ostream& os, const abstract&) -> std::ostream& {
    return os;
  }
};

namespace lll {
namespace fmt {
template <> struct formatter<abstract> : ostream_formatter {};
}  // namespace fmt
}  // namespace lll

void format_abstract_compiles(const abstract& a) {
  lll::fmt::format(LAWS3_FMT_COMPILE("{}"), a);
}

TEST(ostream_test, is_formattable) {
  EXPECT_TRUE(lll::fmt::is_formattable<std::string>());
  EXPECT_TRUE(
      lll::fmt::is_formattable<lll::fmt::detail::std_string_view<char>>());
}

struct streamable_and_unformattable {};

auto operator<<(std::ostream& os, streamable_and_unformattable)
    -> std::ostream& {
  return os << "foo";
}

TEST(ostream_test, streamed) {
  EXPECT_FALSE(lll::fmt::is_formattable<streamable_and_unformattable>());
  EXPECT_EQ(lll::fmt::format(
                "{}", lll::fmt::streamed(streamable_and_unformattable())),
            "foo");
}

TEST(ostream_test, closed_ofstream) {
  std::ofstream ofs;
  lll::fmt::print(ofs, "discard");
}

struct unlocalized {};

auto operator<<(std::ostream& os, unlocalized) -> std::ostream& {
  return os << 12345;
}

namespace lll {
namespace fmt {
template <> struct formatter<unlocalized> : ostream_formatter {};
}  // namespace fmt
}  // namespace lll

TEST(ostream_test, unlocalized) {
  auto loc = get_locale("en_US.UTF-8");
  std::locale::global(loc);
  EXPECT_EQ(lll::fmt::format(loc, "{}", unlocalized()), "12345");
}
