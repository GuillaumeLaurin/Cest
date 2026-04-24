#include <cest/core.hpp>

#include <string>
#include <vector>

// Same type

CEST_MATCHER(isEven, int, [](int v) { return v % 2 == 0; }, "even number");

CEST_MATCHER(
    isPositive, double, [](double v) { return v > 0.0; }, "positive number");

CEST_MATCHER(
    isPalindrome, std::string,
    [](const std::string &s) {
      std::string r(s.rbegin(), s.rend());
      return s == r;
    },
    "palindrome string");

TEST_SUITE("CEST_MATCHER: isEven (int)") {
  cest::describe("basic pass / fail", []() {
    cest::it("passes for an even integer", []() { cest::expect(4).isEven(); });
    cest::it("passes for zero", []() { cest::expect(0).isEven(); });
    cest::it("fails for an odd integer", []() {
      cest::expect(cest::Void([]() { cest::expect(3).isEven(); })).toThrow();
    });
  });

  cest::describe(".Not()", []() {
    cest::it(".Not() passes for an odd integer",
             []() { cest::expect(7).Not().isEven(); });
    cest::it(".Not() fails for an even integer", []() {
      cest::expect(cest::Void([]() {
        cest::expect(2).Not().isEven();
      })).toThrow();
    });
  });

  cest::describe("AssertionError message", []() {
    cest::it("message contains the matcher name", []() {
      bool caught = false;
      try {
        cest::expect(3).isEven();
      } catch (const cest::AssertionError &e) {
        caught = true;
        cest::expect(std::string(e.what())).toMatch("isEven");
      }
      cest::expect(caught).toBeTruthy();
    });

    cest::it(".Not() message contains '.not.'", []() {
      bool caught = false;
      try {
        cest::expect(4).Not().isEven();
      } catch (const cest::AssertionError &e) {
        caught = true;
        cest::expect(std::string(e.what())).toMatch("not");
      }
      cest::expect(caught).toBeTruthy();
    });
  });
}

TEST_SUITE("CEST_MATCHER: isPositive (double)") {
  cest::describe("basic pass / fail", []() {
    cest::it("passes for a positive double",
             []() { cest::expect(3.14).isPositive(); });
    cest::it("fails for zero", []() {
      cest::expect(cest::Void([]() {
        cest::expect(0.0).isPositive();
      })).toThrow();
    });
    cest::it("fails for a negative double", []() {
      cest::expect(cest::Void([]() {
        cest::expect(-1.0).isPositive();
      })).toThrow();
    });
  });

  cest::describe(".Not()", []() {
    cest::it(".Not() passes for a negative double",
             []() { cest::expect(-0.001).Not().isPositive(); });
    cest::it(".Not() fails for a positive double", []() {
      cest::expect(cest::Void([]() {
        cest::expect(1.0).Not().isPositive();
      })).toThrow();
    });
  });
}

TEST_SUITE("CEST_MATCHER: isPalindrome (std::string)") {
  cest::describe("basic pass / fail", []() {
    cest::it("passes for 'racecar'",
             []() { cest::expect(std::string("racecar")).isPalindrome(); });
    cest::it("passes for a single character",
             []() { cest::expect(std::string("a")).isPalindrome(); });
    cest::it("passes for an empty string",
             []() { cest::expect(std::string("")).isPalindrome(); });
    cest::it("fails for 'hello'", []() {
      cest::expect(cest::Void([]() {
        cest::expect(std::string("hello")).isPalindrome();
      })).toThrow();
    });
  });

  cest::describe(".Not()", []() {
    cest::it(".Not() passes for 'hello'",
             []() { cest::expect(std::string("hello")).Not().isPalindrome(); });
    cest::it(".Not() fails for 'level'", []() {
      cest::expect(cest::Void([]() {
        cest::expect(std::string("level")).Not().isPalindrome();
      })).toThrow();
    });
  });
}

TEST_SUITE("CEST_MATCHER: expect() overload dispatch") {
  // Vérifie que la surcharge custom ne capture pas les types sans matcher,
  // et que l'overload générique reste disponible pour int / double / string
  // même quand des matchers custom existent pour ces types.
  cest::describe("generic expect still works alongside custom matchers", []() {
    cest::it("generic toBe still works for int",
             []() { cest::expect(42).toBe(42); });
    cest::it("generic toBe still works for double",
             []() { cest::expect(1.5).toBe(1.5); });
    cest::it("generic toMatch still works for string",
             []() { cest::expect(std::string("racecar")).toMatch("race"); });
  });
}