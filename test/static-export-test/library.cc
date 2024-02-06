#include <3laws/fmt/compile.hpp>

__attribute__((visibility("default"))) std::string foo() {
  return lll::fmt::format(LAWS3_FMT_COMPILE("foo bar {}"), 4242);
}
