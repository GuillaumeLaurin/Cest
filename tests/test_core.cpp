#include "cest/core.hpp"

#define EXPECT_THROWS(expr) cest::expect(cest::Void([&]() { expr; })).toThrow()

TEST_SUITE("expect: toBe / toEqual") {
  cest::describe("toBe", []() {
    cest::it("passes for equal values", []() {
      cest::expect(42).toBe(42);
      cest::expect(std::string("hello")).toBe(std::string("hello"));
    });
    cest::it("fails for different values",
             []() { EXPECT_THROWS(cest::expect(1).toBe(2)); });
    cest::it(".Not() passes for different values",
             []() { cest::expect(1).Not().toBe(2); });
    cest::it(".Not() fails for equal values",
             []() { EXPECT_THROWS(cest::expect(5).Not().toBe(5)); });
  });

  cest::describe("toEqual", []() {
    cest::it("passes for identical integers",
             []() { cest::expect(100).toEqual(100); });
    cest::it("fails for different integers",
             []() { EXPECT_THROWS(cest::expect(1).toEqual(2)); });
  });
}

TEST_SUITE("expect: toBeTruthy / toBeFalsy") {
  cest::describe("toBeTruthy", []() {
    cest::it("passes for true", []() { cest::expect(true).toBeTruthy(); });
    cest::it("passes for non-zero int",
             []() { cest::expect(42).toBeTruthy(); });
    cest::it("fails for false",
             []() { EXPECT_THROWS(cest::expect(false).toBeTruthy()); });
    cest::it(".Not() passes for false",
             []() { cest::expect(false).Not().toBeTruthy(); });
  });

  cest::describe("toBeFalsy", []() {
    cest::it("passes for false", []() { cest::expect(false).toBeFalsy(); });
    cest::it("passes for 0", []() { cest::expect(0).toBeFalsy(); });
    cest::it("fails for true",
             []() { EXPECT_THROWS(cest::expect(true).toBeFalsy()); });
    cest::it(".Not() passes for true",
             []() { cest::expect(true).Not().toBeFalsy(); });
  });
}

TEST_SUITE("expect: toBeGreaterThan / toBeLessThan") {
  cest::describe("toBeGreaterThan", []() {
    cest::it("passes when value is greater",
             []() { cest::expect(10).toBeGreaterThan(5); });
    cest::it("fails when value is smaller",
             []() { EXPECT_THROWS(cest::expect(3).toBeGreaterThan(10)); });
    cest::it(".Not() passes when value is smaller",
             []() { cest::expect(3).Not().toBeGreaterThan(10); });
  });

  cest::describe("toBeLessThan", []() {
    cest::it("passes when value is smaller",
             []() { cest::expect(2).toBeLessThan(5); });
    cest::it("fails when value is greater",
             []() { EXPECT_THROWS(cest::expect(9).toBeLessThan(3)); });
    cest::it(".Not() passes when value is greater",
             []() { cest::expect(9).Not().toBeLessThan(3); });
  });
}

TEST_SUITE("expect: toContain") {
  cest::describe("vector of integers", []() {
    const std::vector<int> v = {1, 2, 3, 4};

    cest::it("passes when element is present",
             [v]() { cest::expect(v).toContain(3); });
    cest::it("fails when element is absent",
             [v]() { EXPECT_THROWS(cest::expect(v).toContain(99)); });
    cest::it(".Not() passes when element is absent",
             [v]() { cest::expect(v).Not().toContain(99); });
    cest::it(".Not() fails when element is present",
             [v]() { EXPECT_THROWS(cest::expect(v).Not().toContain(1)); });
  });

  cest::describe("vector of strings", []() {
    const std::vector<std::string> v = {"foo", "bar", "baz"};

    cest::it("passes when string is present",
             [v]() { cest::expect(v).toContain(std::string("bar")); });
    cest::it(".Not() passes when string is absent",
             [v]() { cest::expect(v).Not().toContain(std::string("qux")); });
  });
}

TEST_SUITE("expect(fn): toThrow / toThrowType") {
  cest::describe("toThrow", []() {
    cest::it("passes when function throws", []() {
      cest::expect(cest::Void([]() {
        throw std::runtime_error("boom");
      })).toThrow();
    });
    cest::it("fails when function does not throw", []() {
      bool threw = false;
      try {
        cest::expect(cest::Void([]() {})).toThrow();
      } catch (const cest::AssertionError &) {
        threw = true;
      }
      cest::expect(threw).toBeTruthy();
    });
    cest::it(".Not() passes when function does not throw",
             []() { cest::expect(cest::Void([]() {})).Not().toThrow(); });
    cest::it(".Not() fails when function throws", []() {
      bool threw = false;
      try {
        cest::expect(cest::Void([]() { throw std::runtime_error("x"); }))
            .Not()
            .toThrow();
      } catch (const cest::AssertionError &) {
        threw = true;
      }
      cest::expect(threw).toBeTruthy();
    });
  });

  cest::describe("toThrowType", []() {
    cest::it("passes when the correct type is thrown", []() {
      cest::expect(cest::Void([]() {
        throw std::runtime_error("err");
      })).toThrowType<std::runtime_error>();
    });
    cest::it("fails when a different type is thrown", []() {
      bool threw = false;
      try {
        cest::expect(cest::Void([]() {
          throw std::logic_error("logic");
        })).toThrowType<std::runtime_error>();
      } catch (const cest::AssertionError &) {
        threw = true;
      }
      cest::expect(threw).toBeTruthy();
    });
    cest::it(".Not() passes when the expected type is NOT thrown", []() {
      cest::expect(cest::Void([]() { throw std::logic_error("other"); }))
          .Not()
          .toThrowType<std::runtime_error>();
    });
    cest::it("fails when nothing is thrown but a type is expected", []() {
      bool threw = false;
      try {
        cest::expect(cest::Void([]() {})).toThrowType<std::runtime_error>();
      } catch (const cest::AssertionError &) {
        threw = true;
      }
      cest::expect(threw).toBeTruthy();
    });
  });
}

TEST_SUITE("Hooks: beforeAll / afterAll") {
  cest::describe("beforeAll initializes before all tests", []() {
    static int counter = 0;

    cest::beforeAll([]() { counter = 10; });

    cest::it("sees the value set by beforeAll",
             []() { cest::expect(counter).toBe(10); });
    cest::it("value is unchanged between tests",
             []() { cest::expect(counter).toBe(10); });
  });

  cest::describe("afterAll runs without error", []() {
    [[maybe_unused]] static int torn_down = 0;
    cest::afterAll([]() { torn_down = 99; });
    cest::it("test runs normally", []() { cest::expect(true).toBeTruthy(); });
  });
}

TEST_SUITE("Hooks: beforeEach / afterEach") {
  cest::describe("beforeEach resets state between tests", []() {
    static int value = 0;

    cest::beforeEach([]() { value = 5; });

    cest::it("first test: value is 5", []() {
      cest::expect(value).toBe(5);
      value = 999;
    });
    cest::it("second test: value is reset to 5",
             []() { cest::expect(value).toBe(5); });
  });

  cest::describe("afterEach runs after each test", []() {
    static int calls = 0;

    cest::afterEach([]() { ++calls; });

    cest::it("first test", []() { cest::expect(1).toBe(1); });
    cest::it("afterEach has been called once after the first test",
             []() { cest::expect(calls).toBe(1); });
  });
}

TEST_SUITE("nested describe: hook inheritance") {
  cest::describe("parent", []() {
    static int hook_log = 0;

    cest::beforeEach([]() { hook_log += 1; });

    cest::it("parent test sees hook_log >= 1",
             []() { cest::expect(hook_log).toBeGreaterThan(0); });

    cest::describe("child", []() {
      cest::beforeEach([]() { hook_log += 10; });

      cest::it("inherits parent beforeEach and runs its own",
               []() { cest::expect(hook_log).toBeGreaterThan(10); });
    });
  });
}

TEST_SUITE("scalar types: double") {
  cest::describe("double", []() {
    cest::it("toBeGreaterThan",
             []() { cest::expect(3.14).toBeGreaterThan(3.0); });
    cest::it("toBeLessThan", []() { cest::expect(1.5).toBeLessThan(2.0); });
  });
}

TEST_SUITE("AssertionError: formatted message") {
  cest::describe("message content", []() {
    cest::it("toBe includes both values in the message", []() {
      bool caught = false;
      try {
        cest::expect(1).toBe(2);
      } catch (const cest::AssertionError &e) {
        caught = true;
        std::string msg = e.what();
        cest::expect(msg).toContain('1');
        cest::expect(msg).toContain('2');
      }
      cest::expect(caught).toBeTruthy();
    });

    cest::it(".not. appears in a negated assertion message", []() {
      bool caught = false;
      try {
        cest::expect(5).Not().toBe(5);
      } catch (const cest::AssertionError &e) {
        caught = true;
        std::string msg = e.what();
        cest::expect(msg.find("not") != std::string::npos).toBeTruthy();
      }
      cest::expect(caught).toBeTruthy();
    });

    cest::it("toEqual also produces an AssertionError", []() {
      bool caught = false;
      try {
        cest::expect(1).Not().toEqual(1);
      } catch (const cest::AssertionError &) {
        caught = true;
      }
      cest::expect(caught).toBeTruthy();
    });
  });
}

TEST_SUITE("it.skip — individual test skipping") {
  cest::describe("skip marks a test as skipped without running its body", []() {
    static bool body_ran = false;

    cest::it.skip("skipped test — body must not execute", []() {
      body_ran = true; // must never be reached
    });

    cest::it("body_ran is still false after the skipped test",
             []() { cest::expect(body_ran).toBeFalsy(); });
  });

  cest::describe("non-skipped siblings still run", []() {
    static int ran = 0;

    cest::it("runs (1)", []() { ++ran; });
    cest::it.skip("skipped — does not increment ran", []() { ++ran; });
    cest::it("runs (2)", []() { ++ran; });

    cest::it("ran equals 2 — only non-skipped tests executed",
             []() { cest::expect(ran).toBe(2); });
  });

  cest::describe("beforeEach is NOT called for skipped tests", []() {
    static int hook_calls = 0;
    static int test_calls = 0;

    cest::beforeEach([]() { ++hook_calls; });

    cest::it("normal test (hook fires)", []() { ++test_calls; });
    cest::it.skip("skipped (hook must not fire)", []() { ++test_calls; });
    cest::it("normal test again (hook fires)", []() { ++test_calls; });

    cest::it("hook fired exactly twice — once per non-skipped test", []() {
      // The beforeEach for this verification test also fires, so hook_calls
      // will be 3 here (once per non-skipped test including this one).
      // What matters: it did NOT fire for the skipped test.
      cest::expect(hook_calls).toBe(3);
      cest::expect(test_calls).toBe(2);
    });
  });
}

TEST_SUITE("describe.skip — entire suite skipping") {
  cest::describe("describe.skip marks all tests in the suite as skipped", []() {
    static bool inner_ran = false;

    cest::describe.skip("skipped suite", []() {
      cest::it("must not run", []() { inner_ran = true; });
      cest::it("also must not run", []() { inner_ran = true; });
    });

    cest::it("inner_ran is false — skipped suite body never executed",
             []() { cest::expect(inner_ran).toBeFalsy(); });
  });

  cest::describe("describe.skip propagates to nested describes", []() {
    static bool deep_ran = false;

    cest::describe.skip("outer skipped", []() {
      cest::describe("nested inside skipped outer", []() {
        cest::it("must not run", []() { deep_ran = true; });
      });
    });

    cest::it("deep_ran is false — nesting inside skipped describe is skipped",
             []() { cest::expect(deep_ran).toBeFalsy(); });
  });

  cest::describe("sibling describes of a skipped suite run normally", []() {
    static int sibling_ran = 0;

    cest::describe.skip("skipped sibling", []() {
      cest::it("must not run", []() { ++sibling_ran; });
    });

    cest::describe("active sibling", []() {
      cest::it("runs normally", []() { ++sibling_ran; });

      cest::it("sibling_ran is 1 — only the active sibling ran",
               []() { cest::expect(sibling_ran).toBe(1); });
    });
  });
}