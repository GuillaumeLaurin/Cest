#define CEST_MAIN
#include <cest.hpp>
#include <mocks.hpp>

#include <string>

// ---------------------------------------------------------------------------
// Targets to be hotpatched.
// No CEST_MOCKABLE annotation needed — Cest::Mock disables inlining globally.
// ---------------------------------------------------------------------------

namespace {

int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }
const char *greet() { return "hello"; }

struct Calculator {
  int compute(int x) { return x * 10; }
  int constant() const { return 42; }
};

int mockAdd(int, int) { return 999; }
int mockMultiply(int a, int b) {
  return a + b;
} // intentional bug: + instead of *
const char *mockGreet() { return "patched"; }

int mockCompute(Calculator *, int) { return -1; }
int mockConstant(const Calculator *) { return 0; }

} // namespace

// ---------------------------------------------------------------------------

TEST_SUITE("hotpatch: free functions") {
  cest::describe("basic patch and restore", []() {
    cest::it("returns the original value before patching",
             []() { cest::expect(add(2, 3)).toBe(5); });

    cest::it("returns the mocked value while the guard is alive", []() {
      auto guard = cest::hotpatch(&add, &mockAdd);
      cest::expect(add(2, 3)).toBe(999);
      cest::expect(add(100, 200)).toBe(999);
    });

    cest::it("restores the original when the guard goes out of scope", []() {
      {
        auto guard = cest::hotpatch(&add, &mockAdd);
        cest::expect(add(2, 3)).toBe(999);
      }
      cest::expect(add(2, 3)).toBe(5);
    });
  });

  cest::describe("multiple independent patches", []() {
    cest::it("two functions can be patched at the same time", []() {
      auto g1 = cest::hotpatch(&add, &mockAdd);
      auto g2 = cest::hotpatch(&multiply, &mockMultiply);

      cest::expect(add(1, 1)).toBe(999);
      cest::expect(multiply(3, 4)).toBe(7); // 3 + 4, not 3 * 4
    });

    cest::it("both restore independently when guards die", []() {
      {
        auto g1 = cest::hotpatch(&add, &mockAdd);
        auto g2 = cest::hotpatch(&multiply, &mockMultiply);
      }
      cest::expect(add(1, 1)).toBe(2);
      cest::expect(multiply(3, 4)).toBe(12);
    });
  });

  cest::describe("functions returning pointers", []() {
    cest::it("patches a function returning const char*", []() {
      auto guard = cest::hotpatch(&greet, &mockGreet);
      cest::expect(std::string(greet())).toBe(std::string("patched"));
    });

    cest::it("restores correctly after pointer-returning patch", []() {
      {
        auto guard = cest::hotpatch(&greet, &mockGreet);
      }
      cest::expect(std::string(greet())).toBe(std::string("hello"));
    });
  });
}

TEST_SUITE("hotpatch: HotpatchGuard lifecycle") {
  cest::describe("explicit restore()", []() {
    cest::it("restores immediately when called manually", []() {
      auto guard = cest::hotpatch(&add, &mockAdd);
      cest::expect(add(2, 3)).toBe(999);

      guard.restore();
      cest::expect(add(2, 3)).toBe(5);
    });

    cest::it("restore() is idempotent", []() {
      auto guard = cest::hotpatch(&add, &mockAdd);
      guard.restore();
      guard.restore(); // should be a no-op
      cest::expect(add(2, 3)).toBe(5);
    });
  });

  cest::describe("move semantics", []() {
    cest::it("moving the guard transfers ownership of the patch", []() {
      cest::HotpatchGuard outer;
      {
        auto inner = cest::hotpatch(&add, &mockAdd);
        cest::expect(add(2, 3)).toBe(999);

        outer = std::move(inner);
        // inner is now empty; outer owns the patch
        cest::expect(add(2, 3)).toBe(999);
      }
      // inner died but it was empty -> patch still active via outer
      cest::expect(add(2, 3)).toBe(999);

      outer = cest::HotpatchGuard{};
      cest::expect(add(2, 3)).toBe(5);
    });
  });

  cest::describe("nested patches on the same function", []() {
    cest::it("inner patch overrides outer while alive", []() {
      auto outer = cest::hotpatch(&add, &mockAdd);
      cest::expect(add(2, 3)).toBe(999);

      {
        int (*inner_fn)(int, int) = [](int, int) { return 7; };
        auto inner = cest::hotpatch(&add, +inner_fn);
        cest::expect(add(2, 3)).toBe(7);
      }

      // After inner restores: we're back to outer's patch (last-write-wins).
      cest::expect(add(2, 3)).toBe(999);
    });

    cest::it("outer restore returns to the original code", []() {
      {
        auto outer = cest::hotpatch(&add, &mockAdd);
      }
      cest::expect(add(2, 3)).toBe(5);
    });
  });
}

TEST_SUITE("hotpatch: non-virtual methods") {
  cest::describe("hotpatchMethod on a mutating method", []() {
    cest::it("returns the mocked value while the guard is alive", []() {
      Calculator c;
      auto guard = cest::hotpatchMethod(&Calculator::compute, &mockCompute);
      cest::expect(c.compute(5)).toBe(-1);
    });

    cest::it("affects all instances of the class (the code is shared)", []() {
      Calculator c1, c2;
      auto guard = cest::hotpatchMethod(&Calculator::compute, &mockCompute);
      cest::expect(c1.compute(5)).toBe(-1);
      cest::expect(c2.compute(99)).toBe(-1);
    });

    cest::it("restores when the guard dies", []() {
      Calculator c;
      {
        auto guard = cest::hotpatchMethod(&Calculator::compute, &mockCompute);
      }
      cest::expect(c.compute(5)).toBe(50);
    });
  });

  cest::describe("hotpatchMethod on a const method", []() {
    cest::it("patches and restores correctly", []() {
      Calculator c;
      cest::expect(c.constant()).toBe(42);

      {
        auto guard = cest::hotpatchMethod(&Calculator::constant, &mockConstant);
        cest::expect(c.constant()).toBe(0);
      }

      cest::expect(c.constant()).toBe(42);
    });
  });
}