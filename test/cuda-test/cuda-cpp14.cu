//  Direct NVCC command line example:
//
//  nvcc ./cuda-cpp14.cu -x cu -I"../include" -l"fmtd" -L"../build/Debug" \
//       -std=c++14 -Xcompiler /std:c++14 -Xcompiler /Zc:__cplusplus

// Ensure that we are using the latest C++ standard for NVCC
// The version is C++14
//
// https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#c-cplusplus-language-support
// https://en.cppreference.com/w/cpp/preprocessor/replace#Predefined_macros
static_assert(__cplusplus >= 201402L, "expect C++ 2014 for nvcc");

#include <cuda.h>

#include <3laws/fmt/base.hpp>
#include <iostream>

extern auto make_message_cpp() -> std::string;
extern auto make_message_cuda() -> std::string;

int main() {
  std::cout << make_message_cuda() << std::endl;
  std::cout << make_message_cpp() << std::endl;
}

auto make_message_cuda() -> std::string {
  return lll::fmt::format("nvcc compiler \t: __cplusplus == {}", __cplusplus);
}
