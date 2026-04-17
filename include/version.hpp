#ifndef VERSION_HPP
#define VERSION_HPP

#ifndef RC_INVOKED
#include "cest/macros/system_header.hpp"
CEST_SYSTEM_HEADER
#endif

#include "cest/macros/stringify.hpp"

#define CEST_VERSION_MAJOR 0
#define CEST_VERSION_MINOR 3
#define CEST_VERSION_BUGFIX 0
#define CEST_VERSION_BUILD 0

#define CEST_VERSION_STATUS ""

#if CEST_VERSION_BUILD != 0
#define CEST_PROJECT_VERSION                                                   \
  CEST_STRINGIFY(CEST_VERSION_MAJOR.CEST_VERSION_MINOR.CEST_VERSION_BUGFIX     \
                     .CEST_VERSION_BUILD)                                      \
  CEST_VERSION_STATUS
#else
#define CEST_PROJECT_VERSION                                                   \
  CEST_STRINGIFY(CEST_VERSION_MAJOR.CEST_VERSION_MINOR.CEST_VERSION_BUGFIX)    \
  CEST_VERSION_STATUS
#endif

/* Version Legend:
   M = Major, m = minor, p = patch, t = tweak, s = status ; [] - excluded if 0
 */

// Format - M.m.p.t (used in Windows RC file)
#define CEST_FILEVERSION_STR                                                   \
  CEST_STRINGIFY(CEST_VERSION_MAJOR.CEST_VERSION_MINOR.CEST_VERSION_BUGFIX     \
                     .CEST_VERSION_BUILD)
// Format - M.m.p[.t]-s
#define CEST_VERSION_STR CEST_PROJECT_VERSION
// Format - vM.m.p[.t]-s
#define CEST_VERSION_STR_2 "v" CEST_PROJECT_VERSION

/*! Version number macro, can be used to check API compatibility, format -
 * MMmmpp. */
#define CEST_VERSION                                                           \
  (CEST_VERSION_MAJOR * 10000 + CEST_VERSION_MINOR * 100 + CEST_VERSION_BUGFIX)

/*! Compute the HEX representation from the given version numbers (for
   comparison). Can be used like: #if CEST_VERSION_HEX >=
   CEST_VERSION_CHECK(0, 37, 3) */
#define CEST_VERSION_CHECK(major, minor, bugfix)                               \
  ((major << 16) | (minor << 8) | (bugfix))

/*! HEX representation of the current Cest version (for comparison).
  CEST_VERSION_HEX is (major << 16) | (minor << 8) | bugfix. */
#define CEST_VERSION_HEX                                                       \
  CEST_VERSION_CHECK(CEST_VERSION_MAJOR, CEST_VERSION_MINOR,                   \
                     CEST_VERSION_BUGFIX)

#endif // VERSION_HPP
