module;

// Put all implementation-provided headers into the global module fragment
// to prevent attachment to this module.
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <locale>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <variant>
#include <vector>
#include <version>

#if __has_include(<cxxabi.h>)
#  include <cxxabi.h>
#endif
#if defined(_MSC_VER) || defined(__MINGW32__)
#  include <intrin.h>
#endif
#if defined __APPLE__ || defined(__FreeBSD__)
#  include <xlocale.h>
#endif
#if __has_include(<winapifamily.h>)
#  include <winapifamily.h>
#endif
#if (__has_include(<fcntl.h>) || defined(__APPLE__) || \
     defined(__linux__)) &&                            \
    (!defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP))
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  ifndef _WIN32
#    include <unistd.h>
#  else
#    include <io.h>
#  endif
#endif
#ifdef _WIN32
#  if defined(__GLIBCXX__)
#    include <ext/stdio_filebuf.h>
#    include <ext/stdio_sync_filebuf.h>
#  endif
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

export module fmt;

#define LAWS3_FMT_EXPORT export
#define LAWS3_FMT_BEGIN_EXPORT export {
#define LAWS3_FMT_END_EXPORT }

// If you define LAWS3_FMT_ATTACH_TO_GLOBAL_MODULE
//  - all declarations are detached from module 'fmt'
//  - the module behaves like a traditional static library, too
//  - all library symbols are mangled traditionally
//  - you can mix TUs with either importing or #including the {fmt} API
#ifdef LAWS3_FMT_ATTACH_TO_GLOBAL_MODULE
extern "C++" {
#endif

#ifndef LAWS3_FMT_OS
#  define LAWS3_FMT_OS 1
#endif

// All library-provided declarations and definitions must be in the module
// purview to be exported.
#include "3laws/fmt/args.hpp"
#include "3laws/fmt/chrono.hpp"
#include "3laws/fmt/color.hpp"
#include "3laws/fmt/compile.hpp"
#include "3laws/fmt/format.hpp"
#if LAWS3_FMT_OS
#  include "fmt/os.hpp"
#endif
#include "3laws/fmt/printf.hpp"
#include "3laws/fmt/std.hpp"
#include "3laws/fmt/xchar.hpp"

#ifdef LAWS3_FMT_ATTACH_TO_GLOBAL_MODULE
}
#endif

// gcc doesn't yet implement private module fragments
#if !LAWS3_FMT_GCC_VERSION
module :private;
#endif

#include "format.cc"
#if LAWS3_FMT_OS
#  include "os.cc"
#endif
