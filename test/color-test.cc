// Formatting library for C++ - color tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "3laws/fmt/color.hpp"

#include <iterator>  // std::back_inserter

#include "gtest-extra.h"  // EXPECT_WRITE, EXPECT_THROW_MSG

TEST(color_test, text_style) {
  EXPECT_FALSE(lll::fmt::text_style().has_foreground());
  EXPECT_FALSE(lll::fmt::text_style().has_background());
  EXPECT_FALSE(lll::fmt::text_style().has_emphasis());

  EXPECT_TRUE(fg(lll::fmt::rgb(0)).has_foreground());
  EXPECT_FALSE(fg(lll::fmt::rgb(0)).has_background());
  EXPECT_FALSE(fg(lll::fmt::rgb(0)).has_emphasis());
  EXPECT_TRUE(bg(lll::fmt::rgb(0)).has_background());
  EXPECT_FALSE(bg(lll::fmt::rgb(0)).has_foreground());
  EXPECT_FALSE(bg(lll::fmt::rgb(0)).has_emphasis());

  EXPECT_TRUE((fg(lll::fmt::rgb(0xFFFFFF)) | bg(lll::fmt::rgb(0xFFFFFF)))
                  .has_foreground());
  EXPECT_TRUE((fg(lll::fmt::rgb(0xFFFFFF)) | bg(lll::fmt::rgb(0xFFFFFF)))
                  .has_background());
  EXPECT_FALSE((fg(lll::fmt::rgb(0xFFFFFF)) | bg(lll::fmt::rgb(0xFFFFFF)))
                   .has_emphasis());

  EXPECT_EQ(fg(lll::fmt::rgb(0x000000)) | fg(lll::fmt::rgb(0x000000)),
            fg(lll::fmt::rgb(0x000000)));
  EXPECT_EQ(fg(lll::fmt::rgb(0x00000F)) | fg(lll::fmt::rgb(0x00000F)),
            fg(lll::fmt::rgb(0x00000F)));
  EXPECT_EQ(fg(lll::fmt::rgb(0xC0F000)) | fg(lll::fmt::rgb(0x000FEE)),
            fg(lll::fmt::rgb(0xC0FFEE)));

  EXPECT_THROW_MSG(
      fg(lll::fmt::terminal_color::black) | fg(lll::fmt::terminal_color::black),
      lll::fmt::format_error, "can't OR a terminal color");
  EXPECT_THROW_MSG(
      fg(lll::fmt::terminal_color::black) | fg(lll::fmt::terminal_color::white),
      lll::fmt::format_error, "can't OR a terminal color");
  EXPECT_THROW_MSG(
      bg(lll::fmt::terminal_color::black) | bg(lll::fmt::terminal_color::black),
      lll::fmt::format_error, "can't OR a terminal color");
  EXPECT_THROW_MSG(
      bg(lll::fmt::terminal_color::black) | bg(lll::fmt::terminal_color::white),
      lll::fmt::format_error, "can't OR a terminal color");
  EXPECT_THROW_MSG(
      fg(lll::fmt::terminal_color::black) | fg(lll::fmt::color::black),
      lll::fmt::format_error, "can't OR a terminal color");
  EXPECT_THROW_MSG(
      bg(lll::fmt::terminal_color::black) | bg(lll::fmt::color::black),
      lll::fmt::format_error, "can't OR a terminal color");

  EXPECT_NO_THROW(fg(lll::fmt::terminal_color::white) |
                  bg(lll::fmt::terminal_color::white));
  EXPECT_NO_THROW(fg(lll::fmt::terminal_color::white) |
                  bg(lll::fmt::rgb(0xFFFFFF)));
  EXPECT_NO_THROW(fg(lll::fmt::terminal_color::white) | lll::fmt::text_style());
  EXPECT_NO_THROW(bg(lll::fmt::terminal_color::white) | lll::fmt::text_style());
}

TEST(color_test, format) {
  EXPECT_EQ(lll::fmt::format(lll::fmt::text_style(), "no style"), "no style");
  EXPECT_EQ(lll::fmt::format(fg(lll::fmt::rgb(255, 20, 30)), "rgb(255,20,30)"),
            "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
  EXPECT_EQ(lll::fmt::format(
                fg(lll::fmt::rgb(255, 0, 0)) | fg(lll::fmt::rgb(0, 20, 30)),
                "rgb(255,20,30)"),
            "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
  EXPECT_EQ(
      lll::fmt::format(fg(lll::fmt::rgb(0, 0, 0)) | fg(lll::fmt::rgb(0, 0, 0)),
                       "rgb(0,0,0)"),
      "\x1b[38;2;000;000;000mrgb(0,0,0)\x1b[0m");
  EXPECT_EQ(lll::fmt::format(fg(lll::fmt::color::blue), "blue"),
            "\x1b[38;2;000;000;255mblue\x1b[0m");
  EXPECT_EQ(
      lll::fmt::format(fg(lll::fmt::color::blue) | bg(lll::fmt::color::red),
                       "two color"),
      "\x1b[38;2;000;000;255m\x1b[48;2;255;000;000mtwo color\x1b[0m");
  EXPECT_EQ(lll::fmt::format(lll::fmt::emphasis::bold, "bold"),
            "\x1b[1mbold\x1b[0m");
  EXPECT_EQ(lll::fmt::format(lll::fmt::emphasis::faint, "faint"),
            "\x1b[2mfaint\x1b[0m");
  EXPECT_EQ(lll::fmt::format(lll::fmt::emphasis::italic, "italic"),
            "\x1b[3mitalic\x1b[0m");
  EXPECT_EQ(lll::fmt::format(lll::fmt::emphasis::underline, "underline"),
            "\x1b[4munderline\x1b[0m");
  EXPECT_EQ(lll::fmt::format(lll::fmt::emphasis::blink, "blink"),
            "\x1b[5mblink\x1b[0m");
  EXPECT_EQ(lll::fmt::format(lll::fmt::emphasis::reverse, "reverse"),
            "\x1b[7mreverse\x1b[0m");
  EXPECT_EQ(lll::fmt::format(lll::fmt::emphasis::conceal, "conceal"),
            "\x1b[8mconceal\x1b[0m");
  EXPECT_EQ(
      lll::fmt::format(lll::fmt::emphasis::strikethrough, "strikethrough"),
      "\x1b[9mstrikethrough\x1b[0m");
  EXPECT_EQ(
      lll::fmt::format(fg(lll::fmt::color::blue) | lll::fmt::emphasis::bold,
                       "blue/bold"),
      "\x1b[1m\x1b[38;2;000;000;255mblue/bold\x1b[0m");
  EXPECT_EQ(lll::fmt::format(lll::fmt::emphasis::bold, "bold error"),
            "\x1b[1mbold error\x1b[0m");
  EXPECT_EQ(lll::fmt::format(fg(lll::fmt::color::blue), "blue log"),
            "\x1b[38;2;000;000;255mblue log\x1b[0m");
  EXPECT_EQ(lll::fmt::format(lll::fmt::text_style(), "hi"), "hi");
  EXPECT_EQ(lll::fmt::format(fg(lll::fmt::terminal_color::red), "tred"),
            "\x1b[31mtred\x1b[0m");
  EXPECT_EQ(lll::fmt::format(bg(lll::fmt::terminal_color::cyan), "tcyan"),
            "\x1b[46mtcyan\x1b[0m");
  EXPECT_EQ(
      lll::fmt::format(fg(lll::fmt::terminal_color::bright_green), "tbgreen"),
      "\x1b[92mtbgreen\x1b[0m");
  EXPECT_EQ(lll::fmt::format(bg(lll::fmt::terminal_color::bright_magenta),
                             "tbmagenta"),
            "\x1b[105mtbmagenta\x1b[0m");
  EXPECT_EQ(lll::fmt::format(fg(lll::fmt::terminal_color::red), "{}", "foo"),
            "\x1b[31mfoo\x1b[0m");
  EXPECT_EQ(lll::fmt::format(
                "{}{}", lll::fmt::styled("red", fg(lll::fmt::color::red)),
                lll::fmt::styled("bold", lll::fmt::emphasis::bold)),
            "\x1b[38;2;255;000;000mred\x1b[0m\x1b[1mbold\x1b[0m");
  EXPECT_EQ(
      lll::fmt::format(
          "{}", lll::fmt::styled("bar", fg(lll::fmt::color::blue) |
                                            lll::fmt::emphasis::underline)),
      "\x1b[4m\x1b[38;2;000;000;255mbar\x1b[0m");
}

TEST(color_test, format_to) {
  auto out = std::string();
  lll::fmt::format_to(std::back_inserter(out), fg(lll::fmt::rgb(255, 20, 30)),
                      "rgb(255,20,30){}{}{}", 1, 2, 3);
  EXPECT_EQ(lll::fmt::to_string(out),
            "\x1b[38;2;255;020;030mrgb(255,20,30)123\x1b[0m");
}

TEST(color_test, print) {
  EXPECT_WRITE(
      stdout, lll::fmt::print(fg(lll::fmt::rgb(255, 20, 30)), "rgb(255,20,30)"),
      "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
}
