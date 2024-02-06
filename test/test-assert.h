// Formatting library for C++ - test version of LAWS3_FMT_ASSERT
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.hpp.

#ifndef LAWS3_FMT_TEST_ASSERT_H_
#define LAWS3_FMT_TEST_ASSERT_H_

#include <stdexcept>

void throw_assertion_failure(const char* message);
#define LAWS3_FMT_ASSERT(condition, message) \
  if (!(condition)) throw_assertion_failure(message);

#include "gtest/gtest.h"

class assertion_failure : public std::logic_error {
 public:
  explicit assertion_failure(const char* message) : std::logic_error(message) {}

 private:
  virtual void avoid_weak_vtable();
};

void assertion_failure::avoid_weak_vtable() {}

// We use a separate function (rather than throw directly from LAWS3_FMT_ASSERT)
// to avoid GCC's -Wterminate warning when LAWS3_FMT_ASSERT is used in a
// destructor.
inline void throw_assertion_failure(const char* message) {
  throw assertion_failure(message);
}

// Expects an assertion failure.
#define EXPECT_ASSERT(stmt, message)                      \
  LAWS3_FMT_TEST_THROW_(stmt, assertion_failure, message, \
                        GTEST_NONFATAL_FAILURE_)

#endif  // LAWS3_FMT_TEST_ASSERT_H_
