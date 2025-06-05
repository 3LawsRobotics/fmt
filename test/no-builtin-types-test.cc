// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "gtest/gtest.h"

#if !defined(__GNUC__) || __GNUC__ >= 5
#  define LAWS3_FMT_BUILTIN_TYPES 0
#  include "3laws/fmt/format.hpp"

TEST(no_builtin_types_test, format) {
  EXPECT_EQ(lll::fmt::format("{}", 42), "42");
  EXPECT_EQ(lll::fmt::format("{}", 42L), "42");
}

TEST(no_builtin_types_test, double_is_custom_type) {
  double d = 42;
  auto args = lll::fmt::make_format_args(d);
  EXPECT_EQ(lll::fmt::format_args(args).get(0).type(),
            lll::fmt::detail::type::custom_type);
}
#endif
