#define CEST_MAIN
#include <cest.hpp>
#include <mocks.hpp>

#include <string>

// ---------------------------------------------------------------------------
// Targets — every hotpatchable function MUST be annotated with CEST_MOCKABLE
// because Cest::MockStrict does not disable inlining globally.
// ---------------------------------------------------------------------------

namespace {

CEST_MOCKABLE int divide(int a, int b) { return a / b; }
CEST_MOCKABLE int subtract(int a, int b) { return a - b; }
CEST_MOCKABLE const char *status() { return "running"; }

struct Service {
  CEST_MOCKABLE int fetch(int id) { return id * 2; }
  CEST_MOCKABLE int version() const { return 1; }
};

int mockDivide(int, int) { return 0; }
int mockSubtract(int a, int b) { return a + b; } // intentional swap
const char *mockStatus() { return "stopped"; }

int mockFetch(Service *, int) { return -42; }
int mockVersion(const Service *) { return 999; }

} // namespace

// ---------------------------------------------------------------------------

TEST_SUITE("strict hotpatch: annotated free functions") {
  cest::describe("CEST_MOCKABLE-annotated function", []() {
    cest::it("returns the original value before patching",
             []() { cest::expect(divide(10, 2)).toBe(5); });

    cest::it("returns the mocked value while the guard is alive", []() {
      auto guard = cest::hotpatch(&divide, &mockDivide);
      cest::expect(divide(10, 2)).toBe(0);
      cest::expect(divide(100, 1)).toBe(0);
    });

    cest::it("restores the original after the guard dies", []() {
      {
        auto guard = cest::hotpatch(&divide, &mockDivide);
      }
      cest::expect(divide(10, 2)).toBe(5);
    });
  });

  cest::describe("function returning a pointer", []() {
    cest::it("patches and restores correctly", []() {
      cest::expect(std::string(status())).toBe(std::string("running"));

      {
        auto guard = cest::hotpatch(&status, &mockStatus);
        cest::expect(std::string(status())).toBe(std::string("stopped"));
      }

      cest::expect(std::string(status())).toBe(std::string("running"));
    });
  });
}

TEST_SUITE("strict hotpatch: annotated methods") {
  cest::describe("non-virtual annotated method", []() {
    cest::it("patches the method while the guard is alive", []() {
      Service s;
      auto guard = cest::hotpatchMethod(&Service::fetch, &mockFetch);
      cest::expect(s.fetch(10)).toBe(-42);
    });

    cest::it("restores the method when the guard dies", []() {
      Service s;
      {
        auto guard = cest::hotpatchMethod(&Service::fetch, &mockFetch);
      }
      cest::expect(s.fetch(10)).toBe(20);
    });
  });

  cest::describe("const-qualified annotated method", []() {
    cest::it("patches and restores correctly", []() {
      Service s;
      cest::expect(s.version()).toBe(1);

      {
        auto guard = cest::hotpatchMethod(&Service::version, &mockVersion);
        cest::expect(s.version()).toBe(999);
      }

      cest::expect(s.version()).toBe(1);
    });
  });
}

TEST_SUITE("strict hotpatch: combined patches") {
  cest::describe("free function and method patched together", []() {
    cest::it("both mocks are active simultaneously", []() {
      Service s;
      auto g1 = cest::hotpatch(&subtract, &mockSubtract);
      auto g2 = cest::hotpatchMethod(&Service::fetch, &mockFetch);

      cest::expect(subtract(10, 3)).toBe(13); // 10 + 3 because mock swaps op
      cest::expect(s.fetch(5)).toBe(-42);
    });

    cest::it("both restore independently", []() {
      Service s;
      {
        auto g1 = cest::hotpatch(&subtract, &mockSubtract);
        auto g2 = cest::hotpatchMethod(&Service::fetch, &mockFetch);
      }
      cest::expect(subtract(10, 3)).toBe(7);
      cest::expect(s.fetch(5)).toBe(10);
    });
  });
}