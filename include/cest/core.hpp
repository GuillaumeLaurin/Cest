#ifndef CORE_HPP
#define CORE_HPP
// -*- C++ -*- Header-only

/**
 * @file include/core.hpp
 * This is the core of the Cest testing library.
 */
#include "cest/type_traits.hpp"

#include "cest/types.hpp"

#include "cest/utils/suite.hpp"

#include "cest/utils/test_case.hpp"

#include "cest/utils/colors.hpp"

#include "cest/reporters/i_reporter.hpp"

#include "cest/reporters/console_reporter.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif // defined(_WIN32) || defined(_WIN64)

namespace cest {

namespace detail {

inline bool endsWith(const std::string &str, const std::string &suffix) {
  if (str.length() < suffix.length())
    return false;
  return str.compare(str.length() - suffix.length(), suffix.length(), suffix) ==
         0;
}
} // namespace detail

/**
 * @brief Error that we raise when assertion failed.
 */
struct AssertionError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

namespace detail {
/**
 * @brief Converts safely a type T to string if the type is streamable
 * @returns a string
 */
template <typename T> std::string toStringSafe(const T &v) {
  if constexpr (is_container_v<T> && !std::is_same_v<T, std::string>) {
    std::string result = "[";
    auto iter = v.begin();
    while (iter != v.end()) {
      result += toStringSafe(*iter);
      if (++iter != v.end()) {
        result += ", ";
      }
    }
    return result + "]";
  } else if constexpr (is_streamable<T>::value) {
    std::ostringstream os;
    os << v;
    return os.str();
  } else if constexpr (std::is_same_v<T, bool>) {
    return v ? "true" : "false";
  } else if constexpr (std::is_same_v<T, char>) {
    return std::string(1, v);
  } else if constexpr (std::is_arithmetic_v<T>) {
    return std::to_string(v);
  } else if constexpr (std::is_convertible_v<T, std::string>) {
    return std::string(v);
  } else {
    return "<non-printable>";
  }
}

} // namespace detail

template <typename T> class PartialType {
  struct FieldCheck {
    std::function<bool(const T &)> predicate;
    std::function<std::string(const T &)> describe;
  };

public:
  template <typename FieldType>
  PartialType &field(FieldType T::*ptr, FieldType value) {
    Checks.push_back({[ptr, value](const T &obj) { return obj.*ptr == value; },
                      [ptr, value](const T &obj) {
                        std::ostringstream os;
                        os << " expected: " << value << "\n"
                           << " received: " << obj.*ptr;
                        return os.str();
                      }});
    return *this;
  }

  std::optional<std::string> firstFailure(const T &obj) const {
    for (auto &check : Checks) {
      if (!check.predicate(obj))
        return check.describe(obj);
    }
    return std::nullopt;
  }

private:
  std::vector<FieldCheck> Checks;
};

template <typename Actual, typename Derived> class AbsExpectation {
public:
  AbsExpectation(Actual value, bool negated = false)
      : Value(value), Negated(negated) {}

  virtual ~AbsExpectation() = default;

  Derived Not() const { return Derived(this->Value, !this->Negated); }

protected:
  Actual Value;
  bool Negated;

  void fail(const char *matcher, const std::string &expected) const {
    std::ostringstream os;
    os << "expect(" << detail::toStringSafe(this->Value) << ")"
       << (this->Negated ? ".not." : ".") << matcher << "(" << expected << ")";
    throw AssertionError(os.str());
  }
};

template <typename Actual>
class Expectation : public AbsExpectation<Actual, Expectation<Actual>> {
public:
  Expectation(Actual value, bool negated = false)
      : AbsExpectation<Actual, Expectation<Actual>>(value, negated) {}

  template <typename Expected> void toBe(const Expected &expected) const {
    bool eq;
    if constexpr (std::is_floating_point_v<Actual> &&
                  std::is_floating_point_v<Expected>) {
      eq = (std::memcmp(&this->Value, &expected, sizeof(Actual)) == 0);
    } else {
      eq = (this->Value == expected);
    }
    if (eq == this->Negated)
      this->fail("toBe", detail::toStringSafe(expected));
  }

  template <typename Expected> void toEqual(const Expected &expected) const {
    bool eq;
    if constexpr (std::is_floating_point_v<Actual> &&
                  std::is_floating_point_v<Expected>) {
      eq = (std::memcmp(&this->Value, &expected, sizeof(Actual)) == 0);
    } else {
      eq = (this->Value == expected);
    }
    if (eq == this->Negated)
      this->fail("toEqual", detail::toStringSafe(expected));
  }

  template <typename Expected>
  void toStrictEqual(const Expected &expected) const {
    using A = std::remove_cv_t<std::remove_reference_t<Actual>>;
    using E = std::remove_cv_t<std::remove_reference_t<Expected>>;

    if constexpr (!std::is_same_v<A, E>) {
      if (!this->Negated)
        this->fail("toStrictEqual", detail::toStringSafe(expected));
      return;
    } else {
      bool eq = deepEqual(this->Value, expected);

      if (eq == this->Negated) {
        if constexpr (is_container_v<A> && is_container_v<E>) {
          this->fail("toStrictEqual", containerDiff(expected));
        } else {
          this->fail("toStrictEqual", detail::toStringSafe(expected));
        }
      }
    }
  }

  void toBeTruthy() const {
    bool t = static_cast<bool>(this->Value);
    if (t == this->Negated)
      this->fail("toBeTruthy", "truthy value");
  }

  void toBeFalsy() const {
    bool t = static_cast<bool>(this->Value);
    if ((!t) == this->Negated)
      this->fail("toBeFalsy", "falsy value");
  }

  //
  // Numeric Matchers
  //

  template <typename Expected,
            std::enable_if_t<std::is_arithmetic_v<Actual> &&
                                 std::is_arithmetic_v<Expected>,
                             int> = 0>
  void toBeGreaterThan(const Expected &expected) const {
    bool r = (this->Value > expected);
    if (r == this->Negated)
      this->fail("toBeGreaterThan", detail::toStringSafe(expected));
  }

  template <typename Expected,
            std::enable_if_t<std::is_arithmetic_v<Actual> &&
                                 std::is_arithmetic_v<Expected>,
                             int> = 0>
  void toBeGreaterThanOrEqual(const Expected &expected) const {
    bool r = (this->Value >= expected);
    if (r == this->Negated)
      this->fail("toBeGreaterThanOrEqual", detail::toStringSafe(expected));
  }

  template <typename Expected,
            std::enable_if_t<std::is_arithmetic_v<Actual> &&
                                 std::is_arithmetic_v<Expected>,
                             int> = 0>
  void toBeLessThan(const Expected &expected) const {
    bool r = (this->Value < expected);
    if (r == this->Negated)
      this->fail("toBeLessThan", detail::toStringSafe(expected));
  }

  template <typename Expected,
            std::enable_if_t<std::is_arithmetic_v<Actual> &&
                                 std::is_arithmetic_v<Expected>,
                             int> = 0>
  void toBeLessThanOrEqual(const Expected &expected) const {
    bool r = (this->Value <= expected);
    if (r == this->Negated)
      this->fail("toBeLessThanOrEqual", detail::toStringSafe(expected));
  }

  template <typename A = Actual,
            std::enable_if_t<std::is_floating_point_v<A>, int> = 0>
  void toBeCloseTo(const float &expected, const uint8_t &precision = 2) {
    float tolerance = std::pow(10.f, static_cast<float>(-precision)) / 2.0f;
    bool r = std::abs(this->Value - expected) < tolerance;
    if (r == this->Negated)
      this->fail("toBeCloseTo", detail::toStringSafe(expected) + " ±" +
                                    detail::toStringSafe(tolerance));
  }

  template <typename A = Actual,
            std::enable_if_t<std::is_floating_point_v<A>, int> = 0>
  void toBeCloseTo(const double &expected, const uint8_t &precision = 2) {
    double tolerance = std::pow(10.0, -static_cast<int>(precision)) / 2.0;
    bool r = std::abs(this->Value - expected) < tolerance;
    if (r == this->Negated)
      this->fail("toBeCloseTo", detail::toStringSafe(expected) + " ±" +
                                    detail::toStringSafe(tolerance));
  }

  template <typename A = Actual,
            std::enable_if_t<std::is_floating_point_v<A>, int> = 0>
  void toBeNaN() {
    bool r = std::isnan(this->Value);
    if (r == this->Negated)
      this->fail("toBeNaN", "NaN");
  }

  template <typename A = Actual,
            std::enable_if_t<std::is_floating_point_v<A>, int> = 0>
  void toBeFinite() {
    bool r = std::isfinite(this->Value);
    if (r == this->Negated)
      this->fail("toBeFinite", "finite");
  }

  template <typename A = Actual,
            std::enable_if_t<std::is_floating_point_v<A>, int> = 0>
  void toBeInfinite() {
    bool r = std::isinf(this->Value);
    if (r == this->Negated)
      this->fail("toBeInfinite", "infinite");
  }

  //
  // String Matchers
  //

  template <typename A = Actual,
            std::enable_if_t<std::is_convertible_v<A, std::string>, int> = 0>
  void toMatch(const std::string &sub) {
    bool r = std::string(this->Value).find(sub) != std::string::npos;
    if (r == this->Negated)
      this->fail("toMatch", "\"" + sub + "\"");
  }

  template <typename A = Actual,
            std::enable_if_t<std::is_convertible_v<A, std::string>, int> = 0>
  void toMatch(const char *sub) {
    bool r = std::string(this->Value).find(sub) != std::string::npos;
    if (r == this->Negated)
      this->fail("toMatch", "\"" + std::string(sub) + "\"");
  }

  template <typename A = Actual,
            std::enable_if_t<std::is_convertible_v<A, std::string>, int> = 0>
  void toMatch(const Regex &regex) {
    bool found = false;
    std::smatch matches;
    if (std::regex_search(this->Value, matches, regex.Reg)) {
      if (matches.size()) {
        found = true;
      }
    }
    if (found == this->Negated)
      this->fail("toMatch", "/" + regex.Pattern + "/");
  }

  template <typename A = Actual,
            std::enable_if_t<std::is_convertible_v<A, std::string>, int> = 0>
  void toStartWith(const std::string &prefix) {
    bool r = std::string(this->Value).rfind(prefix, 0) == 0;
    if (r == this->Negated)
      this->fail("toStartWith", detail::toStringSafe(prefix));
  }

  template <typename A = Actual,
            std::enable_if_t<std::is_convertible_v<A, std::string>, int> = 0>
  void toStartWith(const char *prefix) {
    bool r = std::string(this->Value).rfind(prefix, 0) == 0;
    if (r == this->Negated)
      this->fail("toStartWith", detail::toStringSafe(prefix));
  }

  template <typename A = Actual,
            std::enable_if_t<std::is_convertible_v<A, std::string>, int> = 0>
  void toEndWith(const std::string &suffix) {
    bool r = detail::endsWith(std::string(this->Value), suffix);
    if (r == this->Negated)
      this->fail("toEndWith", detail::toStringSafe(suffix));
  }

  template <typename A = Actual,
            std::enable_if_t<std::is_convertible_v<A, std::string>, int> = 0>
  void toEndWith(const char *suffix) {
    bool r = detail::endsWith(std::string(this->Value), suffix);
    if (r == this->Negated)
      this->fail("toEndWith", detail::toStringSafe(suffix));
  }

  //
  // Container Matchers
  //

  template <typename Needle, typename A = Actual,
            std::enable_if_t<is_container_v<A>, int> = 0>
  void toContain(const Needle &needle) const {
    bool found = false;
    for (const auto &actual : this->Value) {
      if (actual == needle) {
        found = true;
        break;
      }
    }
    if (found == this->Negated)
      this->fail("toContain", detail::toStringSafe(needle));
  }

  template <typename A = Actual, std::enable_if_t<is_container_v<A>, int> = 0>
  void toHaveLength(const std::size_t &length) {
    bool r = this->Value.size() == length;
    if (r == this->Negated)
      this->fail("toHaveLength", detail::toStringSafe(length));
  }

  template <
      typename Expected, typename A = Actual,
      std::enable_if_t<is_container_v<A>, int> = 0,
      std::enable_if_t<std::is_convertible_v<Expected, typename A::value_type>,
                       int> = 0>
  void toContainEqual(const Expected &expected) {
    bool found = false;
    for (const auto &elem : this->Value) {
      if (deepEqual(elem, expected)) {
        found = true;
        break;
      }
    }
    if (found == this->Negated)
      this->fail("toContainEqual", detail::toStringSafe(expected));
  }

  template <typename A = Actual, std::enable_if_t<is_container_v<A>, int> = 0>
  void toBeEmpty() {
    bool r = this->Value.empty();

    if (r == this->Negated)
      this->fail("toBeEmpty",
                 "size=" + detail::toStringSafe(this->Value.size()));
  }

  //
  // Partial Object Matchers
  //

  void toMatchObject(const PartialType<Actual> &partial) const {
    auto failure = partial.firstFailure(this->Value);
    bool res = !failure.has_value();
    if (res == this->Negated)
      this->fail("toMatchObject", failure.value_or(""));
  }

private:
  template <typename T, typename U>
  inline bool deepEqual(const T &a, const U &b) const {
    if constexpr (is_container_v<T> && is_container_v<U>) {
      if (a.size() != b.size())
        return false;
      auto itA = a.begin();
      auto itB = b.begin();
      for (; itA != a.end(); ++itA, ++itB)
        if (!deepEqual(*itA, *itB))
          return false;
      return true;
    } else {
      return a == b;
    }
  }

  template <typename Expected>
  std::string containerDiff(const Expected &expected) const {
    std::ostringstream os;
    os << detail::toStringSafe(expected);

    auto itA = this->Value.begin();
    auto itE = expected.begin();
    std::size_t i = 0;

    for (; itA != this->Value.end() && itE != expected.end();
         ++itA, ++itE, ++i) {
      if (!(*itA == *itE)) {
        os << " (first mismatch at index " << i << ": got "
           << detail::toStringSafe(*itA) << ", expected "
           << detail::toStringSafe(*itE) << ")";
        return os.str();
      }
    }

    if (this->Value.size() != expected.size()) {
      os << " (size mismatch: got " << this->Value.size() << ", expected "
         << expected.size() << ")";
    }

    return os.str();
  }
};

class ThrowingExpectation : public AbsExpectation<Void, ThrowingExpectation> {
public:
  ThrowingExpectation(Void value, bool negated = false)
      : AbsExpectation<Void, ThrowingExpectation>(value, negated) {}

  void toThrow() const {
    bool threw = false;
    try {
      this->Value();
    } catch (...) {
      threw = true;
    }
    if (threw == this->Negated) {
      std::ostringstream os;
      os << "expect(Value)" << (this->Negated ? ".not." : ".") << "toThrow()";
      throw AssertionError(os.str());
    }
  }

  template <typename E> void toThrowType() const {
    bool ok = false;
    try {
      this->Value();
    } catch (const E &) {
      ok = true;
    } catch (...) {
    }
    if (ok == this->Negated) {
      std::ostringstream os;
      os << "expect(Value)" << (this->Negated ? ".not." : ".")
         << "toThrowType<E>()";
      throw AssertionError(os.str());
    }
  }

  void toThrowMessage(const std::string &expected) const {
    bool ok = false;
    std::string message = "";
    try {
      this->Value();
    } catch (const std::exception &e) {
      message = e.what();
      if (e.what() == expected)
        ok = true;
    }
    if (ok == this->Negated) {
      std::ostringstream os;
      os << "expect to " << (this->Negated ? "not " : "") << "throw "
         << expected << " but received " << message;
      throw AssertionError(os.str());
    }
  }
};

template <typename T> Expectation<T> expect(T value) {
  return Expectation<T>(std::move(value));
}

inline ThrowingExpectation expect(Void value) {
  return ThrowingExpectation(std::move(value));
}

class Runner {
public:
  static Runner &instance() {
    static Runner r;
    return r;
  }

  /**
   * @brief Add a reporter that will receive run events. Multiple reporters
   *        can be active simultaneously — they are called in registration
   *        order.
   *
   * If no reporter is added, run() automatically installs a
   * ConsoleReporter so existing user code keeps working.
   */
  void addReporter(std::shared_ptr<reporters::IReporter> reporter) {
    Reporters.push_back(std::move(reporter));
  }

  /**
   * @brief Remove all registered reporters.
   */
  void clearReporters() { Reporters.clear(); }

  void beginDescribe(const std::string &name) {
    utils::Suite s;
    s.Name = name;
    s.Skipped = currentSuite().Skipped;
    Stack.push_back(std::move(s));
  }

  void skipSuite() { currentSuite().Skipped = true; }

  void focusSuite() { currentSuite().Focussed = true; }

  void endDescribe() {
    utils::Suite s = std::move(Stack.back());
    Stack.pop_back();
    currentSuite().Children.push_back(std::move(s));
  }

  /**
   * @brief Called by the TEST_SUITE() macro before invoking the user's body.
   *        Records where new top-level suites are about to land so we can
   *        wrap them on endTestSuiteFile().
   */
  void beginTestSuiteFile(const std::string &file) {
    TestSuiteFileStack.push_back(file);
    TestSuiteRootMarkers.push_back(Root.Children.size());
  }

  /**
   * @brief Called by the TEST_SUITE() macro after the user's body has run.
   *        Wraps every suite that was just appended to Root into a single
   *        synthetic Suite (IsTestSuiteRoot=true, File set), so reporters
   *        can render one PASS/FAIL banner per file.
   */
  void endTestSuiteFile() {
    std::size_t startIdx = TestSuiteRootMarkers.back();
    TestSuiteRootMarkers.pop_back();
    std::string file = TestSuiteFileStack.back();
    TestSuiteFileStack.pop_back();

    utils::Suite wrapper;
    wrapper.File = std::move(file);
    wrapper.IsTestSuiteRoot = true;
    for (std::size_t i = startIdx; i < Root.Children.size(); ++i) {
      wrapper.Children.push_back(std::move(Root.Children[i]));
      // Propagate skip/focus from the children up so that focus detection
      // continues to work at the wrapper level.
      if (wrapper.Children.back().Focussed ||
          wrapper.Children.back().HasFocussedDescendant) {
        wrapper.HasFocussedDescendant = true;
      }
    }
    Root.Children.resize(startIdx);
    Root.Children.push_back(std::move(wrapper));
  }

  void addTest(const std::string &name, Void function) {
    bool isSkipped = currentSuite().Skipped;
    currentSuite().Tests.push_back({name, function, isSkipped});
  }

  void skipTest(const std::string &name, Void function) {
    currentSuite().Tests.push_back({name, function, true});
  }

  void focusTest(const std::string &name, Void function) {
    bool isSkipped = currentSuite().Skipped;
    currentSuite().Tests.push_back({name, function, isSkipped, true});
  }

  void addBeforeAll(Void function) {
    currentSuite().BeforeAll.push_back(function);
  }

  void addAfterAll(Void function) {
    currentSuite().AfterAll.push_back(function);
  }

  void addBeforeEach(Void function) {
    currentSuite().BeforeEach.push_back(function);
  }

  void addAfterEach(Void function) {
    currentSuite().AfterEach.push_back(function);
  }

  int run() {
    if (Reporters.empty()) {
      Reporters.push_back(std::make_shared<reporters::ConsoleReporter>());
    }

    Passed = Failed = Skipped = 0;
    HookErrorCount = 0;
    HasFocus = false;
    dryRun(Root);

    for (const auto &r : Reporters)
      r->onRunStart(/*seed=*/0, /*passed=*/true);

    if (HasFocus) {
      for (const auto &r : Reporters)
        r->onFocussed();
    }

    runSuite(Root, 0, {}, {});

    for (const auto &r : Reporters)
      r->onRunEnd(Passed, Failed, Skipped);

    return (Failed == 0 && HookErrorCount == 0) ? 0 : 1;
  }

private:
  utils::Suite Root;
  std::vector<utils::Suite> Stack;
  std::vector<std::shared_ptr<reporters::IReporter>> Reporters;
  std::vector<std::string> TestSuiteFileStack;
  std::vector<std::size_t> TestSuiteRootMarkers;

  int Passed = 0, Failed = 0, Skipped = 0;
  int HookErrorCount = 0;
  bool HasFocus = false;

  Runner() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
  }

  utils::Suite &currentSuite() { return Stack.empty() ? Root : Stack.back(); }

  using HookList = std::vector<Void>;

  /// Run every hook in `hooks`. If any throw, dispatch onHookError to all
  /// reporters and return false. The first failure's message is written to
  /// `errOut`.
  bool runHooks(const HookList &hooks, const utils::Suite &owner,
                const std::string &hookName, std::string &errOut) {
    bool ok = true;
    for (const auto &h : hooks) {
      try {
        h();
      } catch (const std::exception &e) {
        std::string msg = e.what();
        if (ok)
          errOut = hookName + " threw: " + msg;
        for (const auto &r : Reporters)
          r->onHookError(owner, hookName, msg);
        ok = false;
        ++HookErrorCount;
      } catch (...) {
        std::string msg = "unknown throw";
        if (ok)
          errOut = hookName + " threw: " + msg;
        for (const auto &r : Reporters)
          r->onHookError(owner, hookName, msg);
        ok = false;
        ++HookErrorCount;
      }
    }
    return ok;
  }

  void runSuite(const utils::Suite &s, int depth, HookList inheritedBeforeEach,
                HookList inheritedAfterEach) {
    if (HasFocus && !s.Focussed && !s.HasFocussedDescendant) {
      return;
    }

    // Notify reporters that we're entering this suite. The synthetic Root
    // suite (no name, no IsTestSuiteRoot) is silent — we don't fire events
    // for it because reporters don't need to know about it.
    bool isReportable = !s.Name.empty() || s.IsTestSuiteRoot;
    if (isReportable) {
      for (const auto &r : Reporters)
        r->onSuiteStart(s, depth);
    }

    int childDepth = isReportable ? depth + 1 : depth;
    // The IsTestSuiteRoot wrapper is transparent to indentation: its children
    // appear at the same logical depth as if the wrapper didn't exist.
    if (s.IsTestSuiteRoot)
      childDepth = depth;

    // beforeAll
    std::string hookErr;
    if (!s.Skipped) {
      runHooks(s.BeforeAll, s, "beforeAll", hookErr);
    }

    // Build the per-test hook chains.
    HookList effectiveBefore = inheritedBeforeEach;
    HookList effectiveAfter;
    if (!s.Skipped) {
      for (const auto &h : s.BeforeEach)
        effectiveBefore.push_back(h);
      for (const auto &h : s.AfterEach)
        effectiveAfter.push_back(h);
    }
    for (const auto &h : inheritedAfterEach)
      effectiveAfter.push_back(h);

    // Tests
    for (const auto &t : s.Tests) {
      if (HasFocus && !s.Skipped && !s.Focussed && !t.Focussed) {
        continue;
      }

      if (t.Skipped) {
        for (const auto &r : Reporters)
          r->onTestSkip(t);
        ++Skipped;
        continue;
      }

      bool ok = true;
      std::string errMsg;
      std::string beforeEachErr;

      if (!runHooks(effectiveBefore, s, "beforeEach", beforeEachErr)) {
        ok = false;
        errMsg = beforeEachErr;
      }

      auto start = std::chrono::steady_clock::now();
      if (ok) {
        try {
          t.Function();
        } catch (const AssertionError &e) {
          ok = false;
          errMsg = e.what();
        } catch (const std::exception &e) {
          ok = false;
          errMsg = std::string("threw: ") + e.what();
        } catch (...) {
          ok = false;
          errMsg = "unknown throw";
        }
      }
      auto end = std::chrono::steady_clock::now();
      auto duration =
          std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

      std::string afterEachErr;
      if (!runHooks(effectiveAfter, s, "afterEach", afterEachErr)) {
        if (ok) {
          ok = false;
          errMsg = afterEachErr;
        }
      }

      if (ok) {
        for (const auto &r : Reporters)
          r->onTestPass(t, duration);
        ++Passed;
      } else {
        for (const auto &r : Reporters)
          r->onTestFail(t, errMsg, duration);
        ++Failed;
      }
    }

    // Children
    HookList childBefore = inheritedBeforeEach;
    HookList childAfter;
    if (!s.Skipped) {
      for (const auto &h : s.BeforeEach)
        childBefore.push_back(h);
      for (const auto &h : s.AfterEach)
        childAfter.push_back(h);
    }
    for (const auto &h : inheritedAfterEach)
      childAfter.push_back(h);

    for (const auto &c : s.Children) {
      runSuite(c, childDepth, childBefore, childAfter);
    }

    // afterAll
    if (!s.Skipped) {
      std::string afterAllErr;
      runHooks(s.AfterAll, s, "afterAll", afterAllErr);
    }

    if (isReportable) {
      for (const auto &r : Reporters)
        r->onSuiteEnd(s, depth);
    }
  }

  void dryRun(utils::Suite &s) {
    if (s.Focussed) {
      HasFocus = true;
    }

    for (auto &child : s.Children) {
      dryRun(child);

      if (child.Focussed || child.HasFocussedDescendant) {
        s.HasFocussedDescendant = true;
      }
    }

    for (auto &test : s.Tests) {
      if (test.Focussed) {
        HasFocus = true;
        s.HasFocussedDescendant = true;
      }
    }
  }
};

namespace methods {

class Block {
public:
  virtual ~Block() = default;

  virtual void operator()(const std::string &name, const Void &body) = 0;

  virtual void skip(const std::string &name, const Void &body) = 0;

  virtual void only(const std::string &name, const Void &body) = 0;
};

class It : public cest::methods::Block {
public:
  virtual ~It() override = default;

  virtual void operator()(const std::string &name, const Void &body) override {
    Runner::instance().addTest(name, body);
  }

  virtual void skip(const std::string &name, const Void &body) override {
    Runner::instance().skipTest(name, body);
  }

  virtual void only(const std::string &name, const Void &body) override {
    Runner::instance().focusTest(name, body);
  }
};

class Describe : public cest::methods::Block {
public:
  virtual ~Describe() override = default;

  virtual void operator()(const std::string &name, const Void &body) override {
    Runner::instance().beginDescribe(name);
    body();
    Runner::instance().endDescribe();
  }

  virtual void skip(const std::string &name, const Void &body) override {
    Runner::instance().beginDescribe(name);
    Runner::instance().skipSuite();
    body();
    Runner::instance().endDescribe();
  }

  virtual void only(const std::string &name, const Void &body) override {
    Runner::instance().beginDescribe(name);
    Runner::instance().focusSuite();
    body();
    Runner::instance().endDescribe();
  }
};

} // namespace methods

inline cest::methods::Describe describe;
inline cest::methods::It it;

inline void beforeAll(Void function) {
  Runner::instance().addBeforeAll(std::move(function));
}

inline void afterAll(Void function) {
  Runner::instance().addAfterAll(std::move(function));
}

inline void beforeEach(Void function) {
  Runner::instance().addBeforeEach(std::move(function));
}

inline void afterEach(Void function) {
  Runner::instance().addAfterEach(std::move(function));
}

#define CEST_CAT_(a, b) a##b
#define CEST_CAT(a, b) CEST_CAT_(a, b)
#define TEST_SUITE(Name)                                                       \
  static void CEST_CAT(cest_suite_fn_, __LINE__)();                            \
  namespace {                                                                  \
  struct CEST_CAT(CestSuiteReg_, __LINE__) {                                   \
    CEST_CAT(CestSuiteReg_, __LINE__)() {                                      \
      ::cest::Runner::instance().beginTestSuiteFile(__FILE__);                 \
      CEST_CAT(cest_suite_fn_, __LINE__)();                                    \
      ::cest::Runner::instance().endTestSuiteFile();                           \
    }                                                                          \
  };                                                                           \
  static CEST_CAT(CestSuiteReg_, __LINE__)                                     \
      CEST_CAT(cest_suite_reg_, __LINE__);                                     \
  }                                                                            \
  static void CEST_CAT(cest_suite_fn_, __LINE__)()
} // namespace cest

#define CEST_MATCHER(Name, Type, Predicate, Description)                       \
  struct CEST_CAT(Tag_, Name) {};                                              \
  namespace cest {                                                             \
  template <>                                                                  \
  struct has_matcher<Type, CEST_CAT(Tag_, Name)> : std::true_type {};          \
  class CEST_CAT(Expectation, Name) : public Expectation<Type> {               \
  public:                                                                      \
    CEST_CAT(Expectation, Name)(Type value, bool negated = false)              \
        : Expectation<Type>(value, negated) {}                                 \
                                                                               \
    CEST_CAT(Expectation, Name) Not() const {                                  \
      return CEST_CAT(Expectation, Name)(this->Value, !this->Negated);         \
    }                                                                          \
                                                                               \
    void Name() const {                                                        \
      auto pred = Predicate;                                                   \
      bool r = pred(this->Value);                                              \
      if (r == this->Negated)                                                  \
        this->fail(#Name, Description);                                        \
    }                                                                          \
  };                                                                           \
                                                                               \
  template <HasMatcher<CEST_CAT(Tag_, Name)> T>                                \
  inline CEST_CAT(Expectation, Name) expect(T value) {                         \
    return CEST_CAT(Expectation, Name)(std::move(value));                      \
  }                                                                            \
  }

#ifdef CEST_MAIN
int main() { return ::cest::Runner::instance().run(); }
#endif // CEST_MAIN

#endif // CORE_HPP