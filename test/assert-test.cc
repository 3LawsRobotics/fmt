// Formatting library for C++ - LAWS3_FMT_ASSERT test
//
// It is a separate test to minimize the number of EXPECT_DEBUG_DEATH checks
// which are slow on some platforms. In other tests LAWS3_FMT_ASSERT is made to
// throw an exception which is much faster and easier to check.
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.hpp.

#include "3laws/fmt/base.hpp"
#include "gtest/gtest.h"

TEST(assert_test, fail) {
#if GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(LAWS3_FMT_ASSERT(false, "don't panic!"), "don't panic!");
#else
  lll::fmt::print("warning: death tests are not supported\n");
#endif
}

TEST(assert_test, dangling_else) {
  bool test_condition = false;
  bool executed_else = false;
  if (test_condition)
    LAWS3_FMT_ASSERT(true, "");
  else
    executed_else = true;
  EXPECT_TRUE(executed_else);
}
