#ifndef EXPORT_HPP
#define EXPORT_HPP

#include "cest/macros/system_header.hpp"
CEST_SYSTEM_HEADER

#include "cest/macros/export_common.hpp"

#ifdef CEST_BUILDING_SHARED
#define CEST_EXPORT CEST_DECL_EXPORT
#elif defined(CEST_LINKING_SHARED)
#define CEST_EXPORT CEST_DECL_IMPORT
#endif

// Building library archive (static)
#ifndef CEST_EXPORT
#define CEST_EXPORT
#endif

#endif // EXPORT_HPP