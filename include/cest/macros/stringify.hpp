#ifndef STRINGIFY_HPP
#define STRINGIFY_HPP

#ifndef RC_INVOKED
#include "cest/macros/system_header.hpp"
CEST_SYSTEM_HEADER
#endif // RC_INVOKED

#ifndef CEST_STRINGIFY
#define CEST_STRINGIFY_(x) #x
#define CEST_STRINGIFY(x) CEST_STRINGIFY_(x)
#endif // CEST_STRINGIFY

#endif // STRINGIFY_HPP