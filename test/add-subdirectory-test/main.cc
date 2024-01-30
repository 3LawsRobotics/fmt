#include "fmt/base.h"

int main(int argc, char** argv) {
  for (int i = 0; i < argc; ++i) lll::fmt::print("{}: {}\n", i, argv[i]);
}
