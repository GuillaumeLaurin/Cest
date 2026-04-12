#ifndef COMPILER_DETECT_HPP
#define COMPILER_DETECT_HPP

#include "cest/macros/system_header.hpp"
CEST_SYSTEM_HEADER

// Used compiler
// Must be before GNU, because clang claims to be GNU too
#ifdef __clang__
// Apple Clang has other version numbers
#ifdef __apple_build_version__
#define CEST_COMPILER_STRING "Clang " __clang_version__ " (Apple)"
#else
// Clang-cl simulating MSVC
#ifdef _MSC_VER
#define CEST_COMPILER_STRING "Clang-cl " __clang_version__

#if _MSC_VER < 1910
#define CEST_SIMULATED_STRING "MSVC 2015 (" CEST_STRINGIFY(_MSC_VER) ")"
#elif _MSC_VER < 1917
#define CEST_SIMULATED_STRING "MSVC 2017 (" CEST_STRINGIFY(_MSC_VER) ")"
#elif _MSC_VER < 1930
#define CEST_SIMULATED_STRING "MSVC 2019 (" CEST_STRINGIFY(_MSC_VER) ")"
#elif _MSC_VER < 2000
#define CEST_SIMULATED_STRING "MSVC 2022 (" CEST_STRINGIFY(_MSC_VER) ")"
#else
#define CEST_SIMULATED_STRING "MSVC _MSC_VER " CEST_STRINGIFY(_MSC_VER)
#endif
// Normal Clang
#else
#define CEST_COMPILER_STRING "Clang " __clang_version__
#endif
#endif
#elif defined(__GNUC__)
#define CEST_COMPILER_STRING "GCC " __VERSION__
#elif defined(_MSC_VER)
#if _MSC_VER < 1910
#define CEST_COMPILER_STRING "MSVC 2015 (" CEST_STRINGIFY(_MSC_VER) ")"
#elif _MSC_VER < 1917
#define CEST_COMPILER_STRING "MSVC 2017 (" CEST_STRINGIFY(_MSC_VER) ")"
#elif _MSC_VER < 1930
#define CEST_COMPILER_STRING "MSVC 2019 (" CEST_STRINGIFY(_MSC_VER) ")"
#elif _MSC_VER < 2000
#define CEST_COMPILER_STRING "MSVC 2022 (" CEST_STRINGIFY(_MSC_VER) ")"
#else
#define CEST_COMPILER_STRING "MSVC _MSC_VER " CEST_STRINGIFY(_MSC_VER)
#endif
#else
#define CEST_COMPILER_STRING "<unknown compiler>"
#endif

#endif // COMPILER_DETECT_HPP