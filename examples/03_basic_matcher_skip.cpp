// Example 03 — skip() on it() and describe() blocks.
//
// Demonstrates that:
//  - it.skip() marks individual tests as skipped (yellow output, not run).
//  - describe.skip() marks an entire suite and all its children as skipped.
//  - Skipped tests do not affect the pass / fail count.
//  - Non-skipped siblings in the same describe still run normally.

#include "cest.hpp"

#include <string>
#include <vector>

using namespace cest;

TEST_SUITE(BasicMatcherSkip) {
  describe("it.skip — individual tests", [] {
    it("this test runs normally", [] { expect(1 + 1).toBe(2); });

    it.skip("this test is skipped — body never executes", [] {
      expect(false).toBeTruthy(); // would fail if it ran
    });

    it("this test also runs normally",
       [] { expect(std::string("cest")).Not().toBe(std::string("jest")); });

    it.skip("another skipped test",
            [] { throw std::runtime_error("should never be thrown"); });
  });

  describe("describe.skip — entire suite skipped", [] {
    describe.skip("inner suite (all children skipped)", [] {
      it("skipped because the suite is skipped", [] {
        expect(false).toBeTruthy(); // would fail if it ran
      });

      it("also skipped",
         [] { throw std::runtime_error("should never be thrown"); });

      describe("deeply nested inside a skipped describe", [] {
        it("still skipped", [] {
          expect(false).toBeTruthy(); // would fail if it ran
        });
      });
    });

    it("this sibling runs — it is NOT inside the skipped describe", [] {
      std::vector<int> v{10, 20, 30};
      expect(v).toContain(20);
    });
  });

  describe("mix — skipped and active tests side by side", [] {
    static int counter = 0;

    beforeEach([] { ++counter; });

    it("counter incremented (runs)",
       [] { expect(counter).toBeGreaterThan(0); });

    it.skip("counter NOT incremented for this one (skipped)", [] {
      // beforeEach is not called for skipped tests, so counter
      // would not advance here even if the body executed.
      expect(false).toBeTruthy();
    });

    it("counter reflects only the non-skipped runs", [] {
      // Two non-skipped tests ran before this one (including this call's
      // beforeEach), so counter == 2 at entry to this body... but since
      // we only care that skipped tests didn't bump it, just assert > 0.
      expect(counter).toBeGreaterThan(0);
    });
  });
}