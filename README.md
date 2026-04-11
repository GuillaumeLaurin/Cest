# cest

A header-only C++17 test library inspired by Jest, with `describe` / `it` / `expect`, lifecycle hooks, and a two-tier mocking module — **without inheritance**.

## Overview

```cpp
#define CEST_MAIN
#include "cest.hpp"

using namespace cest;

TEST_SUITE(Math) {
    describe("addition", [] {
        it("adds two numbers", [] {
            expect(2 + 2).toBe(4);
        });
    });
}
```

Compile with `-std=c++17 -I include`, run the binary, and you get Jest-style coloured output.

## Matchers

`toBe`, `toEqual`, `toBeTruthy`, `toBeFalsy`, `toBeGreaterThan`, `toBeLessThan`, `toContain`, `toThrow`, and `.Not()` for negation (C++ does not let you use `not` as an identifier in every context, hence the capitalized `Not()`).

## Lifecycle hooks

All four Jest hooks are supported with the usual semantics:

```cpp
describe("my suite", [] {
    beforeAll([]  { /* once, before the first test in this suite */ });
    afterAll([]   { /* once, after the last test in this suite   */ });
    beforeEach([] { /* before every test in this suite           */ });
    afterEach([]  { /* after every test in this suite            */ });

    it("test 1", [] { /* ... */ });
    it("test 2", [] { /* ... */ });

    describe("nested", [] {
        beforeEach([] { /* runs AFTER the outer beforeEach */ });
        afterEach([]  { /* runs BEFORE the outer afterEach */ });
        it("nested test", [] { /* ... */ });
    });
});
```

Rules that `cest` enforces, matching Jest:

- **`beforeAll` / `afterAll`** fire exactly once per suite, wrapping all of that suite's tests and its children's tests.
- **`beforeEach`** chains run **outermost-first**: the parent suite's `beforeEach` runs before the child suite's `beforeEach` before each test.
- **`afterEach`** chains run **innermost-first**: the child's `afterEach` runs before the parent's, after each test.
- **`afterEach` always runs**, even if the test or a `beforeEach` threw — so cleanup is reliable.
- A throwing hook is reported inline and marks the affected test as failed.

## Mocking

There are two paths — they solve two different problems.

### Tier A: `MockFn<Sig>` via dependency injection (fully portable)

No inheritance, no vtable, zero-cost in production. Parameterize your code by a dependency type and inject a `MockFn` in tests.

```cpp
template <typename Sender>
class Notifier {
    Sender send_;
public:
    explicit Notifier(Sender s) : send_(std::move(s)) {}
    void notify(std::string user, std::string msg) { send_(user, msg); }
};

// In a test:
MockFn<void(std::string, std::string)> sender;
Notifier<decltype(sender)> n(sender);
n.notify("alice", "hi");

expect(sender).toHaveBeenCalledTimes(1);
expect(sender).toHaveBeenCalledWith(std::string("alice"), std::string("hi"));
```

Jest-style API available: `mockReturnValue`, `mockReturnValueOnce` (queue), `mockImplementation`, `mockClear`, `mockReset`, `calls()`, `callCount()`. Matchers: `toHaveBeenCalled`, `toHaveBeenCalledTimes`, `toHaveBeenCalledWith`. Mock copies share the call history (internal `shared_ptr`), so passing the mock by value to your system-under-test works as expected.

### Tier B: `hotpatch()` — runtime interception (Linux/macOS, x86_64 + ARM64)

For cases where you **cannot** modify the calling code: free functions from a third-party library, existing non-virtual methods that you do not want to templatize. Rewrites the first instructions of the target function with a jump to your replacement, and restores on guard destruction.

```cpp
int real_multiply(int a, int b) { return a * b; }  // must be noinline!
int fake_multiply(int, int) { return 999; }

{
    auto guard = hotpatch(&real_multiply, &fake_multiply);
    real_multiply(6, 7);  // -> 999
}
real_multiply(6, 7);      // -> 42 (restored)
```

**Caveats to know about:**

- The target function **must** be non-inlined. Compile with `-O0 -fno-inline`, or mark it `__attribute__((noinline))`, or put it in a separate TU built without optimization.
- Does not work on Windows in this build (you would need `VirtualProtect` + x64 encoding — easy to add, omitted to keep the library simple).
- Not thread-safe during the patch itself.
- No "call original" from the replacement: this is a replace-and-restore, not a full detour trampoline.

## Why two tiers?

The original design goal was "cross-platform" + "mock everything (even non-virtual)" + "no inheritance". These three constraints cannot all be satisfied *cleanly* at the same time. Tier A covers 95 % of real unit-testing needs with zero compromise. Tier B catches the remaining 5 % on platforms where hot-patching is reasonable, and fails explicitly elsewhere with a message telling you to use Tier A instead.

## Build

```bash
mkdir build && cd build
cmake .. && cmake --build .
./cest_example
```

Or by hand:

```bash
g++ -std=c++17 -O0 -fno-inline -I include examples/example.cpp -o example
./example
```