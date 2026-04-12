#ifndef EXPORT_COMMON_HPP
#define EXPORT_COMMON_HPP

#include "cest/macros/system_header.hpp"
CEST_SYSTEM_HEADER

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) ||                  \
    defined(__WIN64__) || defined(WIN32) || defined(_WIN32) ||                 \
    defined(__WIN32__) || defined(__NT__)
#define CEST_DECL_EXPORT __declspec(dllexport)
#define CEST_DECL_IMPORT __declspec(dllimport)
#elif __GNUG__ >= 4
#define CEST_DECL_EXPORT __attribute__((visibility("default")))
#define CEST_DECL_IMPORT __attribute__((visibility("default")))
#define CEST_DECL_HIDDEN __attribute__((visibility("hidden")))
#else
#define CEST_DECL_EXPORT
#define CEST_DECL_IMPORT
#endif

#ifndef CEST_DECL_HIDDEN
#define CEST_DECL_HIDDEN
#endif

#endif // EXPORT_COMMON_HPP