#ifndef COLORS_HPP
#define COLORS_HPP

#include <cstdlib>

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
    bool no_color = (_dupenv_s(&val, &len, "NO_COLOR") == 0 && val != nullptr);
    free(val);
    return !no_color;
#else
    return std::getenv("NO_COLOR") == nullptr;
#endif // defined(_WIN32) || defined(_WIN64)
  }();
  return v;
}

// Foreground colors
inline const char *red() { return colorEnabled() ? "\033[31m" : ""; }
inline const char *green() { return colorEnabled() ? "\033[32m" : ""; }
inline const char *yellow() { return colorEnabled() ? "\033[33m" : ""; }
inline const char *gray() { return colorEnabled() ? "\033[90m" : ""; }
inline const char *black() { return colorEnabled() ? "\033[30m" : ""; }
inline const char *white() { return colorEnabled() ? "\033[37m" : ""; }

// Background colors (used by Jest-style PASS/FAIL banners)
inline const char *bgGreen() { return colorEnabled() ? "\033[42m" : ""; }
inline const char *bgRed() { return colorEnabled() ? "\033[41m" : ""; }
inline const char *bgYellow() { return colorEnabled() ? "\033[43m" : ""; }

// Modifiers
inline const char *bold() { return colorEnabled() ? "\033[1m" : ""; }
inline const char *dim() { return colorEnabled() ? "\033[2m" : ""; }
inline const char *italic() { return colorEnabled() ? "\033[3m" : ""; }
inline const char *reset() { return colorEnabled() ? "\033[0m" : ""; }

} // namespace utils

} // namespace cest

#endif // COLORS_HPP