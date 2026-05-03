#ifndef TYPES_HPP
#define TYPES_HPP

#include <functional>
#include <regex>
#include <string>

namespace cest {

using Void = std::function<void()>;

typedef struct Regex {
  Regex(const std::string &pattern) : Pattern(pattern), Reg(pattern) {}

  std::string Pattern;
  std::regex Reg;
} regex_t;

} // namespace cest

#endif // TYPES_HPP