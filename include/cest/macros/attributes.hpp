#ifndef ATTRIBUTES_HPP
#define ATTRIBUTES_HPP

#if defined(_MSC_VER)
#define CEST_NOINLINE __declspec((noinline, noipa))
#elif defined(__GNUC__) || defined(__clang__)
#define CEST_NOINLINE __attribute__((noinline))
#else
#define CEST_NOINLINE
#endif

// Marks a function as a candidate for cest::hotpatch().
//
// When linking against Cest::Mock, this annotation is optional — the build
// disables inlining globally. When linking against Cest::MockStrict, this
// annotation is REQUIRED on any function you intend to hotpatch.
#define CEST_MOCKABLE CEST_NOINLINE

#endif // ATTRIBUTES_HPP