#ifndef ATTRIBUTES_HPP
#define ATTRIBUTES_HPP

#if defined(_MSC_VER) && !defined(__clang__)
#define CEST_NOINLINE_ATTR __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define CEST_NOINLINE_ATTR __attribute__((noinline))
#else
#define CEST_NOINLINE_ATTR
#endif

#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
#define CEST_NOIPA_ATTR __attribute__((noipa))
#else
#define CEST_NOIPA_ATTR
#endif

#define CEST_NOINLINE CEST_NOINLINE_ATTR CEST_NOIPA_ATTR

// Marks a function as a candidate for cest::hotpatch().
//
// When linking against Cest::Mock, this annotation is optional — the build
// disables inlining globally. When linking against Cest::MockStrict, this
// annotation is REQUIRED on any function you intend to hotpatch.
#define CEST_MOCKABLE CEST_NOINLINE

#endif // ATTRIBUTES_HPP