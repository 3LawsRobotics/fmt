// Formatting library for C++ - the core API
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.hpp.

#include <vector>

#include "3laws/fmt/format.hpp"
#include "3laws/fmt/ranges.hpp"
#include "gtest/gtest.h"

// call lll::fmt::format from another translation unit to test ODR
TEST(ranges_odr_test, format_vector) {
  auto v = std::vector<int>{1, 2, 3, 5, 7, 11};
  EXPECT_EQ(lll::fmt::format("{}", v), "[1, 2, 3, 5, 7, 11]");
}
