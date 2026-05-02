#define CEST_MAIN
#include <cest.hpp>
#include <mocks.hpp>

TEST_SUITE("MockFn: basic calls") {
  cest::describe("callCount / wasCalled / wasCalledTimes", []() {
    cest::it("starts at 0 calls", []() {
      cest::MockFn<void()> mock;
      cest::expect(mock.callCount()).toBe(size_t(0));
      cest::expect(mock.wasCalled()).toBeFalsy();
    });

    cest::it("increments callCount on each call", []() {
      cest::MockFn<void()> mock;
      mock();
      mock();
      mock();
      cest::expect(mock.callCount()).toBe(size_t(3));
      cest::expect(mock.wasCalled()).toBeTruthy();
      cest::expect(mock.wasCalledTimes(3)).toBeTruthy();
      cest::expect(mock.wasCalledTimes(2)).toBeFalsy();
    });

    cest::it("records arguments of each call", []() {
      cest::MockFn<void(int, std::string)> mock;
      mock(1, "hello");
      mock(2, "world");
      cest::expect(mock.callCount()).toBe(size_t(2));
      cest::expect(mock.wasCalledWith(1, std::string("hello"))).toBeTruthy();
      cest::expect(mock.wasCalledWith(2, std::string("world"))).toBeTruthy();
      cest::expect(mock.wasCalledWith(3, std::string("nope"))).toBeFalsy();
    });
  });
}

TEST_SUITE("MockFn: return values") {
  cest::describe("mockReturnValue", []() {
    cest::it("returns the default value on every call", []() {
      cest::MockFn<int()> mock;
      mock.mockReturnValue(42);
      cest::expect(mock()).toBe(42);
      cest::expect(mock()).toBe(42);
    });

    cest::it("returns int{} when nothing is configured", []() {
      cest::MockFn<int()> mock;
      cest::expect(mock()).toBe(0);
    });
  });

  cest::describe("mockReturnValueOnce", []() {
    cest::it("consumes the one-time value then falls back to the default",
             []() {
               cest::MockFn<int()> mock;
               mock.mockReturnValue(0);
               mock.mockReturnValueOnce(99);
               cest::expect(mock()).toBe(99);
               cest::expect(mock()).toBe(0);
             });

    cest::it("consumes one-time values in FIFO order", []() {
      cest::MockFn<int()> mock;
      mock.mockReturnValue(-1);
      mock.mockReturnValueOnce(10);
      mock.mockReturnValueOnce(20);
      mock.mockReturnValueOnce(30);
      cest::expect(mock()).toBe(10);
      cest::expect(mock()).toBe(20);
      cest::expect(mock()).toBe(30);
      cest::expect(mock()).toBe(-1);
    });
  });
}

TEST_SUITE("MockFn: mockImplementation") {
  cest::describe("with return value", []() {
    cest::it("executes the custom implementation", []() {
      cest::MockFn<int(int, int)> mock;
      mock.mockImplementation([](int a, int b) { return a + b; });
      cest::expect(mock(3, 4)).toBe(7);
      cest::expect(mock(10, 20)).toBe(30);
    });

    cest::it("Once values take priority over the implementation", []() {
      cest::MockFn<int()> mock;
      mock.mockImplementation([]() { return 100; });
      mock.mockReturnValueOnce(999);
      cest::expect(mock()).toBe(999);
      cest::expect(mock()).toBe(100);
    });
  });

  cest::describe("with void", []() {
    cest::it("executes the implementation and records the call", []() {
      int sideEffect = 0;
      cest::MockFn<void(int)> mock;
      mock.mockImplementation([&sideEffect](int v) { sideEffect = v; });
      mock(42);
      cest::expect(sideEffect).toBe(42);
      cest::expect(mock.callCount()).toBe(size_t(1));
    });
  });
}

TEST_SUITE("MockFn: mockClear / mockReset") {
  cest::describe("mockClear", []() {
    cest::it("clears calls but keeps the return value", []() {
      cest::MockFn<int()> mock;
      mock.mockReturnValue(7);
      mock();
      mock();
      cest::expect(mock.callCount()).toBe(size_t(2));
      mock.mockClear();
      cest::expect(mock.callCount()).toBe(size_t(0));
      cest::expect(mock()).toBe(7);
    });
  });

  cest::describe("mockReset", []() {
    cest::it("clears calls AND the configured return value", []() {
      cest::MockFn<int()> mock;
      mock.mockReturnValue(55);
      mock();
      mock.mockReset();
      cest::expect(mock.callCount()).toBe(size_t(0));
      cest::expect(mock()).toBe(0);
    });

    cest::it("clears the implementation", []() {
      int called = 0;
      cest::MockFn<void()> mock;
      mock.mockImplementation([&called]() { ++called; });
      mock();
      cest::expect(called).toBe(1);
      mock.mockReset();
      mock();
      cest::expect(called).toBe(1);
    });
  });
}

TEST_SUITE("expect(mock): Jest matchers") {
  cest::describe("toHaveBeenCalled", []() {
    cest::it("passes when the mock was called", []() {
      cest::MockFn<void()> mock;
      mock();
      cest::expect(mock).toHaveBeenCalled();
    });

    cest::it("fails when the mock was not called", []() {
      cest::MockFn<void()> mock;
      bool threw = false;
      try {
        cest::expect(mock).toHaveBeenCalled();
      } catch (const cest::AssertionError &) {
        threw = true;
      }
      cest::expect(threw).toBeTruthy();
    });

    cest::it(".Not() passes when the mock was not called", []() {
      cest::MockFn<void()> mock;
      cest::expect(mock).Not().toHaveBeenCalled();
    });

    cest::it(".Not() fails when the mock was called", []() {
      cest::MockFn<void()> mock;
      mock();
      bool threw = false;
      try {
        cest::expect(mock).Not().toHaveBeenCalled();
      } catch (const cest::AssertionError &) {
        threw = true;
      }
      cest::expect(threw).toBeTruthy();
    });
  });

  cest::describe("toHaveBeenCalledTimes", []() {
    cest::it("passes with the correct call count", []() {
      cest::MockFn<void()> mock;
      mock();
      mock();
      cest::expect(mock).toHaveBeenCalledTimes(2);
    });

    cest::it("fails with the wrong call count", []() {
      cest::MockFn<void()> mock;
      mock();
      bool threw = false;
      try {
        cest::expect(mock).toHaveBeenCalledTimes(5);
      } catch (const cest::AssertionError &) {
        threw = true;
      }
      cest::expect(threw).toBeTruthy();
    });

    cest::it(".Not() passes when count does not match", []() {
      cest::MockFn<void()> mock;
      mock();
      cest::expect(mock).Not().toHaveBeenCalledTimes(99);
    });
  });

  cest::describe("toHaveBeenCalledWith", []() {
    cest::it("passes when a call matches the arguments", []() {
      cest::MockFn<void(int, std::string)> mock;
      mock(1, "hi");
      mock(2, "bye");
      cest::expect(mock).toHaveBeenCalledWith(1, std::string("hi"));
    });

    cest::it("fails when no call matches", []() {
      cest::MockFn<void(int)> mock;
      mock(1);
      bool threw = false;
      try {
        cest::expect(mock).toHaveBeenCalledWith(99);
      } catch (const cest::AssertionError &) {
        threw = true;
      }
      cest::expect(threw).toBeTruthy();
    });

    cest::it(".Not() passes when no call matches", []() {
      cest::MockFn<void(int)> mock;
      mock(1);
      cest::expect(mock).Not().toHaveBeenCalledWith(42);
    });
  });
}

TEST_SUITE("MockFn: calls()") {
  cest::describe("argument tuple history", []() {
    cest::it("returns all calls in order", []() {
      cest::MockFn<void(int, int)> mock;
      mock(1, 2);
      mock(3, 4);
      auto c = mock.calls();
      cest::expect(c.size()).toBe(size_t(2));
      cest::expect(std::get<0>(c[0])).toBe(1);
      cest::expect(std::get<1>(c[0])).toBe(2);
      cest::expect(std::get<0>(c[1])).toBe(3);
      cest::expect(std::get<1>(c[1])).toBe(4);
    });

    cest::it("returns an empty vector when never called", []() {
      cest::MockFn<void(int)> mock;
      cest::expect(mock.calls().size()).toBe(size_t(0));
    });
  });
}

TEST_SUITE("MockFn: dependency injection") {
  cest::describe("service with a mocked logger", []() {
    struct Logger {
      cest::MockFn<void(std::string)> log;
    };

    auto runService = [](Logger &l, int v) {
      l.log(v > 0 ? std::string("positive") : std::string("non-positive"));
    };

    cest::it("logs 'positive' for a positive value", [runService]() {
      Logger logger;
      runService(logger, 5);
      cest::expect(logger.log).toHaveBeenCalledWith(std::string("positive"));
    });

    cest::it("logs 'non-positive' for zero", [runService]() {
      Logger logger;
      runService(logger, 0);
      cest::expect(logger.log)
          .toHaveBeenCalledWith(std::string("non-positive"));
    });

    cest::it("log is called exactly once per run", [runService]() {
      Logger logger;
      runService(logger, 42);
      cest::expect(logger.log).toHaveBeenCalledTimes(1);
    });
  });
}

TEST_SUITE("MockFn: non-default-constructible type") {
  cest::describe("throws std::runtime_error when no value is configured", []() {
    struct NonDefault {
      NonDefault() = delete;
      explicit NonDefault(int) {}
      bool operator==(const NonDefault &) const { return false; }
    };

    cest::it("throws std::runtime_error", []() {
      cest::MockFn<NonDefault()> mock;
      bool threw = false;
      try {
        mock();
      } catch (const std::runtime_error &) {
        threw = true;
      }
      cest::expect(threw).toBeTruthy();
    });
  });
}