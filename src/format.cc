// Formatting library for C++
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "3laws/fmt/format-inl.h"

LAWS3_FMT_BEGIN_NAMESPACE
namespace detail {

template LAWS3_FMT_API auto dragonbox::to_decimal(float x) noexcept
    -> dragonbox::decimal_fp<float>;
template LAWS3_FMT_API auto dragonbox::to_decimal(double x) noexcept
    -> dragonbox::decimal_fp<double>;

#ifndef LAWS3_FMT_STATIC_THOUSANDS_SEPARATOR
template LAWS3_FMT_API locale_ref::locale_ref(const std::locale& loc);
template LAWS3_FMT_API auto locale_ref::get<std::locale>() const -> std::locale;
#endif

// Explicit instantiations for char.

template LAWS3_FMT_API auto thousands_sep_impl(locale_ref)
    -> thousands_sep_result<char>;
template LAWS3_FMT_API auto decimal_point_impl(locale_ref) -> char;

template LAWS3_FMT_API void buffer<char>::append(const char*, const char*);

template LAWS3_FMT_API void vformat_to(buffer<char>&, string_view,
                                       typename vformat_args<>::type,
                                       locale_ref);

// Explicit instantiations for wchar_t.

template LAWS3_FMT_API auto thousands_sep_impl(locale_ref)
    -> thousands_sep_result<wchar_t>;
template LAWS3_FMT_API auto decimal_point_impl(locale_ref) -> wchar_t;

template LAWS3_FMT_API void buffer<wchar_t>::append(const wchar_t*,
                                                    const wchar_t*);

}  // namespace detail
LAWS3_FMT_END_NAMESPACE
