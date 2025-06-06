// Header-only configuration test

#include "3laws/fmt/base.hpp"
#include "3laws/fmt/ostream.hpp"
#include "gtest/gtest.h"

#ifndef LAWS3_FMT_HEADER_ONLY
#  error "Not in the header-only mode."
#endif

TEST(header_only_test, format) { EXPECT_EQ(lll::fmt::format("foo"), "foo"); }
