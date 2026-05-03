#ifndef TEST_CASE_HPP
#define TEST_CASE_HPP

#include "cest/types.hpp"

#include <string>

namespace cest {

namespace utils {

struct TestCase {
  std::string Name;
  Void Function;
  bool Skipped = false;
  bool Focussed = false;
};

} // namespace utils

} // namespace cest

#endif // TEST_CASE_HPP