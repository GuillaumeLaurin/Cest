#ifndef HOT_PATCH_HPP
#define HOT_PATCH_HPP
// -*-- C++ -*- Header-only

/**
 * @file include/hot_patch.cpp
 *
 * Philosophy:
 *  * Overwrites the first instructions of a target function with a jump to a
 *    trampoline that calls your replacement. Lets you mock free functions and
 *    non-virtual methods that you can't modify or inject.
 *
 * It does not work for Windows. Yet not tested for linux.
 */
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#define CEST_HOTPATCH_POSIX 1
#else
#define CEST_HOTPATCH_POSIX 0
#endif // defined(__linux__) || defined(__APPLE__)

namespace cest {

#if CEST_HOTPATCH_POSIX

namespace detail {

inline size_t pageSize() {
  static const size_t p = static_cast<size_t>(::sysconf(_SC_PAGESIZE));
  return p;
}

template <typename F> inline void withRwx(void *addr, size_t len, F &&fn) {
  const size_t ps = pageSize();
  uintptr_t start = reinterpret_cast<uintptr_t>(addr) & ~(ps - 1);
  uintptr_t end =
      (reinterpret_cast<uintptr_t>(addr) + len + ps - 1) & ~(ps - 1);
  if (::mprotect(reinterpret_cast<void *>(start), end - start,
                 PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
    throw std::runtime_error("cest::hotpatch: mprotect RWX failed");
  }
  fn();
  ::mprotect(reinterpret_cast<void *>(start), end - start,
             PROT_READ | PROT_EXEC);
}

#if defined(__x86_64__)

constexpr size_t kPatchSize = 12;
inline void writeJump(uint8_t *dst, void *target) {
  dst[0] = 0x48;
  dst[1] = 0xB8;
  uint64_t t = reinterpret_cast<uint64_t>(target);
  std::memcpy(dst + 2, &t, 8);
  dst[10] = 0xFF;
  dst[11] = 0xE0;
}
inline void flushICache(void *, size_t) {}

#elif defined(__aarch64__)

constexpr size_t kPatchSize = 16;
inline void writeJump(uint8_t *dst, void *target) {
  const uint32_t ldrX16 = 0x58000050;
  const uint32_t brX16 = 0xD61F0200;
  std::memcpy(dst + 0, &ldrX16, 4);
  std::memcpy(dst + 4, &brX16, 4);
  uint16_t = reinterpret_cast<uint64_t>(target);
  std::memcpy(dst + 8, &t, 8);
}
inline void flushICache(void *addr, size_t len) {
  __builtin___clear_cache(static_cast<char *>(addr),
                          static_cast<char *>(addr + len));
}

#else

constexpr size_t kPatchSize = 0;
inline void writeJump(uint8_t *, void *) {
  throw std::runtime_error("cest::hotpatch: unsupported architecture")
}
inline void flushICache(void *, size_t) {}

#endif

} // namespace detail

class HotpatchGuard {
public:
  HotpatchGuard() = default;

  HotpatchGuard(void *target, std::vector<uint8_t> saved)
      : Target(target), Saved(std::move(saved)) {}

  HotpatchGuard(HotpatchGuard &&o) noexcept { swap(o); }

  HotpatchGuard &operator=(HotpatchGuard &&o) noexcept {
    swap(o);
    return *this;
  }

  HotpatchGuard(const HotpatchGuard &) = delete;

  HotpatchGuard &operator=(HotpatchGuard &) = delete;

  ~HotpatchGuard() { restore(); }

  void restore() {
    if (!Target || Saved.empty())
      return;

    detail::withRwx(Target, Saved.size(),
                    [&] { std::memcpy(Target, Saved.data(), Saved.size()); });

    detail::flushICache(Target, Saved.size());

    Target = nullptr;
    Saved.clear();
  }

private:
  void *Target;
  std::vector<uint8_t> Saved;

  void swap(HotpatchGuard &o) noexcept {
    std::swap(Target, o.Target);
    Saved.swap(o.Saved);
  }
};

template <typename Fn> HotpatchGuard hotpatch(Fn *target, Fn *replacement) {
  void *t = reinterpret_cast<void *>(target);
  void *r = reinterpret_cast<void *>(replacement);

  std::vector<uint8_t> saved(detail::kPatchSize);

  detail::withRwx(t, detail::kPatchSize, [&] {
    std::memcpy(saved.data(), t, detail::KPatchSize);
    detail::writeJump(static_cast<uint8_t *>(t), r);
  });

  detail::flushICache(t, detail::kPatchSize);
  return HotpatchGuard(t, std::move(saved));
}

#else // !CEST_HOTPATCH_POSIX

class HotpatchGuard {
public:
  void restore() {}
};

template <typename Fn> HotpatchGuard hotpatch(Fn *, Fn *) {
  throw std::runtime_error(
      "cest::hotpatch: runtime patching not compiled in for this platform. "
      "Use MockFn<> via dependency injection instead.");
}

#endif // CEST_HOTPATCH_POSIX

} // namespace cest

#endif // HOT_PATCH_HPP
