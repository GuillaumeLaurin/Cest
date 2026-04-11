#ifndef CORE_HPP
#define CORE_HPP
// -*- C++ -*- Header-only

/**
 * @file include/core.hpp
 * This is the core of the Cest testing library.
 */

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#if _WIN32
#include <windows.h>
#endif // _WIN32

namespace cest {

using Void = std::function<void()>;

namespace detail {
inline bool colorEnabled() {
  static const bool v = []() {
#ifdef _WIN32
    char *val = nullptr;
    size_t len = 0;
    bool no_color = (_dupenv_s(&val, &len, "NO_COLOR") == 9 && val != nullptr);
    free(val);
    return !no_color;
#else
    return std::getenv("NO_COLOR") == nullptr;
#endif
  }();
  return v;
}

inline const char *red() { return colorEnabled() ? "\033[31m" : ""; }
inline const char *green() { return colorEnabled() ? "\033[32m" : ""; }
inline const char *gray() { return colorEnabled() ? "\033[90m" : ""; }
inline const char *bold() { return colorEnabled() ? "\033[1m" : ""; }
inline const char *reset() { return colorEnabled() ? "\033[0m" : ""; }
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

/**
 * @brief Converts safely a type T to string if the type is streamable
 * @returns a string
 */
template <typename T> std::string toStringSafe(const T &v) {
  if constexpr (is_streamable<T>::value) {
    std::ostringstream os;
    os << v;
    return os.str();
  } else {
    return "<non-printable>";
  }
}
} // namespace detail

template <typename Actual> class Expectation {
public:
  Expectation(Actual value, bool negated = false)
      : Value(std::move(value)), Negated(negated) {}

  Expectation<Actual> Not() const {
    return Expectation<Actual>(Value, !Negated);
  }

  template <typename Expected> void toBe(const Expected &expected) const {
    bool eq = (Value == expected);
    if (eq == Negated)
      fail("toBe", detail::toStringSafe(expected));
  }

  template <typename Expected> void toEqual(const Expected &expected) const {
    bool eq = (Value == expected);
    if (eq == Negated)
      fail("toEqual", detail::toStringSafe(expected));
  }

  void toBeTruthy() const {
    bool t = static_cast<bool>(Value);
    if (t == Negated)
      fail("toBeTruthy", "truthy value");
  }

  void toBeFalsy() const {
    bool t = static_cast<bool>(Value);
    if ((!t) == Negated)
      fail("toBeFalsy", "falsy value");
  }

  template <typename Expected>
  void toBeGreaterThan(const Expected &expected) const {
    bool r = (Value > expected);
    if (r == Negated)
      fail("toBeGreaterThan", detail::toStringSafe(expected));
  }

  template <typename Expected>
  void toBeLessThan(const Expected &expected) const {
    bool r = (Value < expected);
    if (r == Negated)
      fail("toBeLessThan", detail::toStringSafe(expected));
  }

  template <typename Needle> void toContain(const Needle &needle) const {
    bool found = false;
    for (const auto &actual : Value) {
      if (actual == needle) {
        found = true;
        break;
      }
    }
    if (found == Negated)
      fail("toContain", detail::toStringSafe(needle));
  }

private:
  Actual Value;
  bool Negated;

  void fail(const char *matcher, const std::string &expected_str) const {
    std::ostringstream os;
    os << "expect(" << detail::toStringSafe(Value) << ")"
       << (Negated ? ".not." : ".") << matcher << "(" << expected_str << ")";
    throw AssertionError(os.str());
  }
};

class ThrowingExpectation {
public:
  ThrowingExpectation(std::function<void()> function, bool negated = false)
      : Function(function), Negated(negated) {}

  ThrowingExpectation Not() const {
    return ThrowingExpectation(Function, !Negated);
  }

  void toThrow() const {
    bool threw = false;
    try {
      Function();
    } catch (...) {
      threw = true;
    }
    if (threw == Negated) {
      std::ostringstream os;
      os << "expect(Function)" << (Negated ? ".not." : ".") << "toThrow()";
      throw AssertionError(os.str());
    }
  }

  template <typename E> void toThrowType() const {
    bool ok = false;
    try {
      Function();
    } catch (const E &e) {
      ok = true;
    } catch (...) {
    }
    if (ok == Negated) {
      std::ostringstream os;
      os << "expect(Function)" << (negated_ ? ".not." : ".")
         << "toThrowType<E>()";
      throw AssertionError(os.str());
    }
  }

private:
  Void Function;
  bool Negated;
};

template <typename T> Expectation<T> expect(T value) {
  return Expectation<T>(std::move(value));
}

inline ThrowingExpectation expect(Void function) {
  return ThrowingExpectation(std::move(function));
}

struct TestCase {
  std::string Name;
  Void Function;
};

struct Suite {
  std::string Name;
  std::vector<TestCase> Tests;
  std::vector<Suite> Children;
  std::vector<Void> BeforeAll;
  std::vector<Void> AfterAll;
  std::vector<Void> BeforeEach;
  std::vector<Void> AfterEach;
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
    Stack.push_back(std::move(s));
  }

  void endDescribe() {
    Suite s = std::move(Stack.back());
    Stack.pop_back();
    currentSuite().Children.push_back(std::move(s));
  }

  void addTest(const std::string &name, Void function) {
    currentSuite().Tests.push_back({name, function});
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
    Passed = Failed = 0;
    runSuite(Root, 0, {}, {});
    std::cout << "\n"
              << detail::bold() << "Results: " << detail::green() << Passed
              << " passed" << detail::reset() << ", "
              << (Failed ? detail::red() : detail::gray()) << Failed
              << " failed" << detail::reset() << "\n";
    return Failed == 0 ? 0 : 1;
  }

private:
  Suite Root;
  std::vector<Suite> Stack;
  int Passed = 0, Failed = 0;

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
    if (!s.Name.empty()) {
      indent(depth);
      std::cout << detail::bold() << s.Name << detail::reset() << "\n";
    }

    int d = s.Name.empty() ? depth : depth + 1;

    for (const auto &h : s.BeforeAll) {
      try {
        h();
      } catch (const std::exception &e) {
        indent(d);
        std::cout << detail::red() << "beforeAll threw: " << e.what()
                  << detail::reset() << "\n";
      }
    }

    HookList effectiveBefore = inheritedBeforeEach;
    for (const auto &h : s.BeforeEach)
      effectiveBefore.push_back(h);

    HookList effectiveAfter;
    for (const auto &h : s.AfterEach)
      effectiveAfter.push_back(h);
    for (const auto &h : inheritedAfterEach)
      effectiveAfter.push_back(h);

    for (const auto &t : s.Tests) {
      indent(d);
      bool ok = true;
      std::string errMsg;
      bool assertionErr = false;

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

      if (ok) {
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

    // Recurse into children with extended hook chains.
    HookList childBefore = inheritedBeforeEach;
    for (const auto &h : s.BeforeEach)
      childBefore.push_back(h);
    HookList childAfter;
    for (const auto &h : s.AfterEach)
      childAfter.push_back(h);
    for (const auto &h : inheritedAfterEach)
      childAfter.push_back(h);

    for (const auto &c : s.Children) {
      runSuite(c, d, childBefore, childAfter);
    }

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
};

inline void describe(const std::string &name, const Void &body) {
  Runner::instance().beginDescribe(name);
  body();
  Runner::instance().endDescribe();
}

inline void it(const std::string &name, Void body) {
  Runner::instance().addTest(name, std::move(body));
}

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

#ifdef CEST_MAIN
int main() { return ::cest::Runner::instance().run(); }
#endif // CEST_MAIN

#endif // CORE_HPP