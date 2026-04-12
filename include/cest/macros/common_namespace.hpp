#ifndef COMMON_NAMESPACE_HPP
#define COMMON_NAMESPACE_HPP

#include "cest/macros/system_header.hpp"
CEST_SYSTEM_HEADER

// User defined namespace
#ifdef CEST_COMMON_NAMESPACE
#define CEST_BEGIN_COMMON_NAMESPACE namespace CEST_COMMON_NAMESPACE {
#define CEST_END_COMMON_NAMESPACE }
#define CEST_PREPEND_NAMESPACE(name) CEST_COMMON_NAMESPACE::name
// User namespace is not defined
#else
#define CEST_BEGIN_COMMON_NAMESPACE
#define CEST_END_COMMON_NAMESPACE
#define CEST_PREPEND_NAMESPACE(name) name
#endif

#endif // COMMON_NAMESPACE_HPP
