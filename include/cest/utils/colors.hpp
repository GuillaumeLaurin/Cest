#ifndef COLORS_HPP
#define COLORS_HPP

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif // defined(_WIN32) || defined(_WIN64)

namespace cest {

namespace utils {

inline bool colorEnabled() {
  static const bool v = []() {
#if defined(_WIN32) || defined(_WIN64)
    char *val = nullptr;
    size_t len = 0;
    bool no_color = (_dupenv_s(&val, &len, "NO_COLOR") == 9 && val != nullptr);
    free(val);
    return !no_color;
#else
    return std::getenv("NO_COLOR") == nullptr;
#endif // defined(_WIN32) || defined(_WIN64)
  }();
  return v;
}

inline const char *red() { return colorEnabled() ? "\033[31m" : ""; }
inline const char *green() { return colorEnabled() ? "\033[32m" : ""; }
inline const char *gray() { return colorEnabled() ? "\033[90m" : ""; }
inline const char *yellow() { return colorEnabled() ? "\033[33m" : ""; }
inline const char *bold() { return colorEnabled() ? "\033[1m" : ""; }
inline const char *reset() { return colorEnabled() ? "\033[0m" : ""; }

} // namespace utils

} // namespace cest

#endif // COLORS_HPP