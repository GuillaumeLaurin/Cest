#ifndef SYSTEM_HEADER_HPP
#define SYSTEM_HEADER_HPP

#ifndef CEST_PRAGMA_SYSTEM_HEADER_OFF
// Clang masquerades as GCC 4.2.0 so it has to be first
#ifdef __clang__
#define CEST_SYSTEM_HEADER _Pragma("clang system_header")
#elif __GNUC__ * 100 + __GNUC_MINOR__ > 301
#define CEST_SYSTEM_HEADER _Pragma("GCC system_header")
#elif defined(_MSC_VER)
#define CEST_SYSTEM_HEADER _Pragma("system_header")
#endif
#endif

#ifndef CEST_SYSTEM_HEADER
#define CEST_SYSTEM_HEADER
#endif

CEST_SYSTEM_HEADER

#endif // SYSTEM_HEADER_HPP
