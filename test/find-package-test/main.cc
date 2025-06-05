#include "3laws/fmt/format.hpp"

int main(int argc, char** argv) {
  for (int i = 0; i < argc; ++i) lll::fmt::print("{}: {}\n", i, argv[i]);
}
