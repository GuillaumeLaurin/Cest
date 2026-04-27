#ifndef CORE_HPP
#define CORE_HPP
// -*- C++ -*- Header-only

/**
 * @file include/core.hpp
 * This is the core of the Cest testing library.
 */

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif // defined(_WIN32) || defined(_WIN64)

namespace cest {

using Void = std::function<void()>;

struct Regex {
  Regex(const std::string &pattern) : Pattern(pattern), Reg(pattern) {}

  std::string Pattern;
  std::regex Reg;
};

namespace detail {
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
template <typename T, typename = void>
struct is_streamable : std::false_type {};

template <typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream &>()
                                             << std::declval<const T &>())>>
    : std::true_type {};

template <typename T, typename = void> struct is_container : std::false_type {};

template <typename T>
struct is_container<T, std::void_t<typename T::value_type, typename T::iterator,
                                   decltype(std::declval<T>().begin()),
                                   decltype(std::declval<T>().end()),
                                   decltype(std::declval<T>().size())>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_container_v = is_container<T>::value;

template <typename T, typename MatcherTag>
struct has_matcher : std::false_type {};

template <typename T, typename MatcherTag>
concept HasMatcher = has_matcher<T, MatcherTag>::value;

/**
 * @brief Converts safely a type T to string if the type is streamable
 * @returns a string
 */
template <typename T> std::string toStringSafe(const T &v) {
  if constexpr (detail::is_container_v<T> && !std::is_same_v<T, std::string>) {
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
        if constexpr (detail::is_container_v<A> && detail::is_container_v<E>) {
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
            std::enable_if_t<detail::is_container_v<A>, int> = 0>
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

  template <typename A = Actual,
            std::enable_if_t<detail::is_container_v<A>, int> = 0>
  void toHaveLength(const std::size_t &length) {
    bool r = this->Value.size() == length;
    if (r == this->Negated)
      this->fail("toHaveLength", detail::toStringSafe(length));
  }

  template <
      typename Expected, typename A = Actual,
      std::enable_if_t<detail::is_container_v<A>, int> = 0,
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

  template <typename A = Actual,
            std::enable_if_t<detail::is_container_v<A>, int> = 0>
  void toBeEmpty() {
    bool r = this->Value.empty();

    if (r == this->Negated)
      this->fail("toBeEmpty",
                 "size=" + detail::toStringSafe(this->Value.size()));
  }

private:
  template <typename T, typename U>
  inline bool deepEqual(const T &a, const U &b) const {
    if constexpr (detail::is_container_v<T> && detail::is_container_v<U>) {
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

struct TestCase {
  std::string Name;
  Void Function;
  bool Skipped = false;
  bool Focussed = false;
};

struct Suite {
  std::string Name;
  std::vector<TestCase> Tests;
  std::vector<Suite> Children;
  std::vector<Void> BeforeAll;
  std::vector<Void> AfterAll;
  std::vector<Void> BeforeEach;
  std::vector<Void> AfterEach;
  bool Skipped = false;
  bool Focussed = false;
  bool HasFocussedDescendant = false;
};

class Runner {
public:
  static Runner &instance() {
    static Runner r;
    return r;
  }

  void beginDescribe(const std::string &name) {
    Suite s;
    s.Name = name;
    s.Skipped = currentSuite().Skipped;
    Stack.push_back(std::move(s));
  }

  void skipSuite() { currentSuite().Skipped = true; }

  void focusSuite() { currentSuite().Focussed = true; }

  void endDescribe() {
    Suite s = std::move(Stack.back());
    Stack.pop_back();
    currentSuite().Children.push_back(std::move(s));
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
    Passed = Failed = Skipped = 0;
    HasFocus = false;
    dryRun(Root);
    if (HasFocus) {
      std::cout << "\n"
                << detail::bold() << detail::yellow() << "Focus mode activated"
                << detail::reset() << "\n"
                << detail::yellow() << "Don't forget to turn it off"
                << detail::reset() << "\n";
    }
    runSuite(Root, 0, {}, {});
    std::cout << "\n"
              << detail::bold() << "Results: " << detail::green() << Passed
              << " passed" << detail::reset() << ", "
              << (Failed ? detail::red() : detail::gray()) << Failed
              << " failed" << detail::reset() << ", "
              << (Skipped ? detail::yellow() : detail::gray()) << Skipped
              << " skipped" << detail::reset() << "\n";
    return Failed == 0 ? 0 : 1;
  }

private:
  Suite Root;
  std::vector<Suite> Stack;
  int Passed = 0, Failed = 0, Skipped = 0;
  bool HasFocus = false;

  Runner() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
  }

  Suite &currentSuite() { return Stack.empty() ? Root : Stack.back(); }

  using HookList = std::vector<Void>;

  void indent(int d) {
    for (int i = 0; i < d; ++i) {
      std::cout << "  ";
    }
  }

  void runSuite(const Suite &s, int depth, HookList inheritedBeforeEach,
                HookList inheritedAfterEach) {
    if (HasFocus && !s.Focussed && !s.HasFocussedDescendant) {
      return;
    }
    if (!s.Name.empty()) {
      indent(depth);
      if (s.Skipped) {
        std::cout << detail::bold() << detail::yellow() << s.Name
                  << detail::reset() << "\n";
      } else {
        std::cout << detail::bold() << s.Name << detail::reset() << "\n";
      }
    }

    int d = s.Name.empty() ? depth : depth + 1;

    if (!s.Skipped) {
      for (const auto &h : s.BeforeAll) {
        try {
          h();
        } catch (const std::exception &e) {
          indent(d);
          std::cout << detail::red() << "beforeAll threw: " << e.what()
                    << detail::reset() << "\n";
        }
      }
    }

    HookList effectiveBefore;
    HookList effectiveAfter;

    if (!s.Skipped) {
      effectiveBefore = inheritedBeforeEach;
      for (const auto &h : s.BeforeEach)
        effectiveBefore.push_back(h);

      for (const auto &h : s.AfterEach)
        effectiveAfter.push_back(h);
      for (const auto &h : inheritedAfterEach)
        effectiveAfter.push_back(h);
    }

    for (const auto &t : s.Tests) {
      if (HasFocus && !s.Skipped && !s.Focussed) {
        continue;
      }

      indent(d);
      bool ok = true;
      std::string errMsg;
      bool assertionErr = false;

      if (!t.Skipped) {

        for (const auto &h : effectiveBefore) {
          try {
            h();
          } catch (const std::exception &e) {
            ok = false;
            errMsg = std::string("beforeEach threw: ") + e.what();
            break;
          }
        }

        if (ok) {
          try {
            t.Function();
          } catch (const AssertionError &e) {
            ok = false;
            assertionErr = true;
            errMsg = e.what();
          } catch (const std::exception &e) {
            ok = false;
            errMsg = std::string("threw: ") + e.what();
          } catch (...) {
            ok = false;
            errMsg = "unknown throw";
          }
        }

        for (const auto &h : effectiveAfter) {
          try {
            h();
          } catch (const std::exception &e) {
            if (ok) {
              ok = false;
              errMsg = std::string("afterEach threw: ") + e.what();
            }
          }
        }
      }

      if (t.Skipped) {
        std::cout << detail::yellow() << "Skipped " << t.Name << detail::reset()
                  << "\n";
        ++Skipped;
      } else if (ok) {
        std::cout << detail::green() << "\xE2\x9C\x93 " << detail::reset()
                  << t.Name << "\n";
        ++Passed;
      } else {
        std::cout << detail::red() << "\xE2\x9C\x97 " << t.Name
                  << detail::reset() << "\n";
        indent(d + 1);
        std::cout << detail::red() << errMsg << detail::reset() << "\n";
        ++Failed;
        (void)assertionErr;
      }
    }

    // Recurse into children with extended hook chains if suite is not skipped.
    HookList childBefore;
    HookList childAfter;

    if (!s.Skipped) {
      childBefore = inheritedBeforeEach;
      for (const auto &h : s.BeforeEach)
        childBefore.push_back(h);

      for (const auto &h : s.AfterEach)
        childAfter.push_back(h);
      for (const auto &h : inheritedAfterEach)
        childAfter.push_back(h);
    }

    for (const auto &c : s.Children) {
      runSuite(c, d, childBefore, childAfter);
    }

    if (!s.Skipped) {
      for (const auto &h : s.AfterAll) {
        try {
          h();
        } catch (const std::exception &e) {
          indent(d);
          std::cout << detail::red() << "afterAll threw: " << e.what()
                    << detail::reset() << "\n";
        }
      }
    }
  }

  void dryRun(Suite &s) {
    if (s.Focussed) {
      HasFocus = true;
    }

    for (auto &child : s.Children) {
      dryRun(child);

      if (child.Focussed || child.HasFocussedDescendant) {
        s.HasFocussedDescendant = true;
      }
    }

    for (auto test : s.Tests) {
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
      ::cest::Runner::instance();                                              \
      CEST_CAT(cest_suite_fn_, __LINE__)();                                    \
    }                                                                          \
  };                                                                           \
  static CEST_CAT(CestSuiteReg_, __LINE__)                                     \
      CEST_CAT(cest_suite_reg_, __LINE__);                                     \
  }                                                                            \
  static void CEST_CAT(cest_suite_fn_, __LINE__)()
} // namespace cest

#define CEST_MATCHER(Name, Type, Predicate, Description)                       \
  struct CEST_CAT(Tag_, Name) {};                                              \
  namespace cest::detail {                                                     \
  template <>                                                                  \
  struct has_matcher<Type, CEST_CAT(Tag_, Name)> : std::true_type {};          \
  }                                                                            \
  namespace cest {                                                             \
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
  template <detail::HasMatcher<CEST_CAT(Tag_, Name)> T>                        \
  inline CEST_CAT(Expectation, Name) expect(T value) {                         \
    return CEST_CAT(Expectation, Name)(std::move(value));                      \
  }                                                                            \
  }

#ifdef CEST_MAIN
int main() { return ::cest::Runner::instance().run(); }
#endif // CEST_MAIN

#endif // CORE_HPP
