#ifndef SUITE_HPP
#define SUITE_HPP

#include "cest/utils/test_case.hpp"

#include "cest/types.hpp"

#include <string>
#include <vector>

namespace cest {

namespace utils {

struct Suite {
  std::string Name;
  std::string File;
  std::vector<TestCase> Tests;
  std::vector<Suite> Children;
  std::vector<Void> BeforeAll;
  std::vector<Void> AfterAll;
  std::vector<Void> BeforeEach;
  std::vector<Void> AfterEach;
  bool Skipped = false;
  bool Focussed = false;
  bool HasFocussedDescendant = false;
  bool IsTestSuiteRoot = false;
};

} // namespace utils

} // namespace cest

#endif // SUITE_HPP