// Self-test and usage examples.
#include "cest.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace cest;

TEST_SUITE(BasicMatcher) {
  describe("expect() matchers", [] {
    it("toBe compares equality", [] {
      expect(2 + 2).toBe(4);
      expect(std::string("hi")).toBe(std::string("hi"));
    });

    it("toBeTruthy / toBeFalsy", [] {
      expect(1).toBeTruthy();
      expect(0).toBeFalsy();
    });

    it("toBeGreaterThan / toBeLessThan", [] {
      expect(10).toBeGreaterThan(3);
      expect(3).toBeLessThan(10);
    });

    it("toContain works on vectors", [] {
      std::vector<int> v{1, 2, 3};
      expect(v).toContain(2);
    });

    it("not.toBe negates", [] { expect(1).Not().toBe(2); });

    it("toThrow detects exceptions", [] {
      expect(std::function<void()>([] {
        throw std::runtime_error("x");
      })).toThrow();
      expect(std::function<void()>([] {
        /* no throw */
      }))
          .Not()
          .toThrow();
    });
  });
}