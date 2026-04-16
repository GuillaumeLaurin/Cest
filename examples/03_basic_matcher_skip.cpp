#include "cest.hpp"

#include <string>
#include <vector>

using namespace cest;

TEST_SUITE(BasicMatcherSkip) {
  describe("it.skip — individual tests", [] {
    it("this test runs normally", [] { expect(1 + 1).toBe(2); });

    it.skip("this test is skipped — body never executes",
            [] { expect(false).toBeTruthy(); });

    it("this test also runs normally",
       [] { expect(std::string("cest")).Not().toBe(std::string("jest")); });

    it.skip("another skipped test",
            [] { throw std::runtime_error("should never be thrown"); });
  });

  describe("describe.skip — entire suite skipped", [] {
    describe.skip("inner suite (all children skipped)", [] {
      it("skipped because the suite is skipped",
         [] { expect(false).toBeTruthy(); });

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

    it.skip("counter NOT incremented for this one (skipped)",
            [] { expect(false).toBeTruthy(); });

    it("counter reflects only the non-skipped runs",
       [] { expect(counter).toBeGreaterThan(0); });
  });
}