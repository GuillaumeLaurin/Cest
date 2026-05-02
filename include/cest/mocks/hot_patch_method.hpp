#ifndef HOT_PATCH_METHOD
#define HOT_PATCH_METHOD

#include "cest/mocks/hot_patch.hpp"

namespace cest {

namespace detail {

// Extract the raw code address from a non-virtual member function pointer.
//
// Non-portable but works on GCC/Clang/MSVC with default ABI settings:
//  * Itanium ABI (Linux/macOS): non-virtual member pointer is { void* fn,
//  ptrdiff_t adj }
//    where fn is a direct code pointer when the method is non-virtual.
//  * MSVC default (single-inheritance class): member pointer is a single void*.
//
// For multiple-inheritance or virtual-inheritance classes under MSVC, the
// member pointer is larger and this extraction will not work. For virtual
// methods on either ABI, use VTableGuard instead.
template <typename MemFn> inline void *memberFnAddress(MemFn mfn) {
  static_assert(sizeof(MemFn) >= sizeof(void *),
                "unexpected member pointer size");
  void *addr = nullptr;
  std::memcpy(&addr, &mfn, sizeof(void *));
  return addr;
}

} // namespace detail

// Patch a non-virtual member function. The replacement is a free function
// whose first parameter is the class pointer (the implicit `this`).
//
// Example:
//   struct Foo { int compute(int x) { return x * 2; } };
//   int mock(Foo*, int) { return 42; }
//   auto guard = cest::hotpatchMethod(&Foo::compute, &mock);
template <typename Class, typename Ret, typename... Args>
HotpatchGuard hotpatchMethod(Ret (Class::*target)(Args...),
                             Ret (*replacement)(Class *, Args...)) {
  void *t = detail::memberFnAddress(target);
  void *r = reinterpret_cast<void *>(replacement);

  std::vector<uint8_t> saved(detail::kPatchSize);
  detail::withRwx(t, detail::kPatchSize, [&] {
    std::memcpy(saved.data(), t, detail::kPatchSize);
    detail::writeJump(static_cast<uint8_t *>(t), r);
  });
  detail::flushICache(t, detail::kPatchSize);
  return HotpatchGuard(t, std::move(saved));
}

// Const-qualified overload.
template <typename Class, typename Ret, typename... Args>
HotpatchGuard hotpatchMethod(Ret (Class::*target)(Args...) const,
                             Ret (*replacement)(const Class *, Args...)) {
  void *t = detail::memberFnAddress(target);
  void *r = reinterpret_cast<void *>(replacement);

  std::vector<uint8_t> saved(detail::kPatchSize);
  detail::withRwx(t, detail::kPatchSize, [&] {
    std::memcpy(saved.data(), t, detail::kPatchSize);
    detail::writeJump(static_cast<uint8_t *>(t), r);
  });
  detail::flushICache(t, detail::kPatchSize);
  return HotpatchGuard(t, std::move(saved));
}

} // namespace cest

#endif // HOT_PATCH_METHOD