#define CEST_MAIN
#include "cest/core.hpp"

TEST_SUITE("it.only - focus mode") {
  cest::describe("only the focused test runs", []() {
    static int ran = 0;

    cest::it("normal test — should be skipped in focus mode", []() { ++ran; });

    cest::it.only("focused test — must run", []() {
      ++ran;
      cest::expect(ran).toBe(1);
    });
  });
}

TEST_SUITE("describe.only - focus mode on suite") {
  cest::describe("skipped suite — not focused", []() {
    cest::it("must not run", []() { cest::expect(true).toBeFalsy(); });
  });

  cest::describe.only("focused suite - must run", []() {
    cest::it("runs normally", []() { cest::expect(42).toBe(42); });

    cest::it("also runs", []() {
      cest::expect(std::string("hello")).toBe(std::string("hello"));
    });
  });
}