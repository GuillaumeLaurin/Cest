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

### Equality

`toBe(expected)` — passes if `value == expected` (using `operator==`).

`toEqual(expected)` — alias for `toBe`; use when the intent is deep-value equality rather than identity.

### Truthiness

`toBeTruthy()` — passes if `static_cast<bool>(value)` is `true`.

`toBeFalsy()` — passes if `static_cast<bool>(value)` is `false`.

### Ordering

`toBeGreaterThan(expected)` — passes if `value > expected`.

`toBeGreaterThanOrEqual(expected)` — passes if `value >= expected`.

`toBeLessThan(expected)` — passes if `value < expected`.

`toBeLessThanOrEqual(expected)` — passes if `value <= expected`.

### Floating-point

`toBeCloseTo(expected, precision = 2)` — passes if `|value - expected| < 10^(-precision) / 2`. The default precision of `2` matches two decimal places (tolerance of `0.005`). Overloads are provided for both `float` and `double`.

```cpp
expect(0.1 + 0.2).toBeCloseTo(0.3);          // precision 2 (default)
expect(1.0 / 3.0).toBeCloseTo(0.333, 3);     // precision 3
```

`toBeNaN()` — passes if the value satisfies `std::isnan`.

`toBeFinite()` — passes if the value satisfies `std::isfinite`.

`toBeInfinite()` — passes if the value satisfies `std::isinf`.

```cpp
expect(std::numeric_limits<double>::quiet_NaN()).toBeNaN();
expect(42.0).toBeFinite();
expect(std::numeric_limits<double>::infinity()).toBeInfinite();
```

### String

These matchers are only available when `Actual` is convertible to `std::string`. They will not compile for numeric or other non-string types.

`toMatch(substring)` — passes if the string contains the given substring. Accepts `std::string` or `const char *`.

`toMatch(regex)` — passes if the string matches the given `cest::Regex`. The pattern is preserved for readable error messages.

`toStartWith(prefix)` — passes if the string begins with the given prefix. Accepts `std::string` or `const char *`.

`toEndWith(suffix)` — passes if the string ends with the given suffix. Accepts `std::string` or `const char *`.

```cpp
expect(std::string("hello world")).toMatch("world");
expect(std::string("hello world")).toMatch(cest::Regex{"^hello"});
expect(std::string("hello world")).toStartWith("hello");
expect(std::string("hello world")).toEndWith("world");

expect(std::string("hello")).Not().toMatch("xyz");
expect(std::string("hello world")).Not().toStartWith("world");
```

### Containers

`toContain(needle)` — passes if any element of the iterable `value` compares equal to `needle`.

### Exceptions

`toThrow()` — passes if the callable throws any exception.

`toThrowType<E>()` — passes if the callable throws an exception of type `E` (caught by `catch (const E &)`).

### Negation

`.Not()` — inverts the sense of any matcher. C++ does not allow `not` as an unqualified identifier in every context, hence the capitalised form.

```cpp
expect(1).Not().toBe(2);
expect(0.0).Not().toBeNaN();
expect(value).Not().toBeInfinite();
```

## Skipping tests and suites

Both `it` and `describe` support a `.skip()` call that marks a test or an entire suite as skipped. Skipped items appear in yellow in the output and are counted separately — they do not affect the pass/fail result.

### `it.skip` — skip a single test

```cpp
describe("my suite", [] {
    it("this runs", [] { expect(1).toBe(1); });

    it.skip("this is skipped — body never executes", [] {
        expect(false).toBeTruthy(); // never reached
    });

    it("this also runs", [] { expect(2).toBe(2); });
});
```

Rules that `cest` enforces for skipped tests, matching Jest:

- The test body is **never called**.
- `beforeEach` / `afterEach` hooks are **not called** for the skipped test.
- `beforeAll` / `afterAll` are unaffected — they still fire once for the suite.
- Non-skipped siblings run normally.

### `describe.skip` — skip an entire suite

```cpp
describe.skip("whole suite is skipped", [] {
    it("will not run", [] { /* ... */ });

    describe("nested inside skipped — also skipped", [] {
        it("will not run either", [] { /* ... */ });
    });
});
```

Rules:

- All tests directly inside the skipped suite are skipped.
- All **nested** `describe` blocks inside the skipped suite are also skipped, recursively.
- Sibling `describe` blocks at the same level are **not** affected.

## Focusing tests and suites

Both `it` and `describe` support a `.only()` call that activates **focus mode**. When focus mode is active, only focused items run — everything else is silently skipped. A yellow banner is printed at the start of the run to remind you that focus mode is on.

> **Warning:** focus mode is a development tool. Never commit `.only()` calls — they will silently suppress most of your test suite in CI.

### `it.only` — focus a single test

```cpp
describe("my suite", [] {
    it("this is skipped in focus mode", [] { /* ... */ });

    it.only("only this test runs", [] {
        expect(6 * 7).toBe(42);
    });

    it("this is also skipped in focus mode", [] { /* ... */ });
});
```

### `describe.only` — focus an entire suite

```cpp
describe.only("only this suite runs", [] {
    it("runs", [] { expect(1).toBe(1); });
    it("also runs", [] { expect(2).toBe(2); });
});

describe("this whole suite is skipped in focus mode", [] {
    it("will not run", [] { /* ... */ });
});
```

### Mixing `it.only` and `describe.only`

Focus mode is global: as soon as any `.only()` appears anywhere in the binary, every non-focused item is suppressed. You can mix `it.only` and `describe.only` freely — all focused items run, everything else does not.

```cpp
describe("suite A", [] {
    it.only("focused test in A — runs", [] { expect(true).toBeTruthy(); });
    it("non-focused test in A — skipped", [] { /* ... */ });
});

describe.only("suite B — all tests run", [] {
    it("runs", [] { expect(42).toBe(42); });
});
```

### Rules that `cest` enforces for focus mode, matching Jest

- A focused test inside a non-focused suite still runs — focus propagates upward.
- A non-focused test inside a focused suite runs — the suite focus covers all its children.
- `beforeEach` / `afterEach` / `beforeAll` / `afterAll` hooks fire normally for tests that run under focus mode.
- Tests and suites marked with `.skip()` inside a focused suite remain skipped.
- The runner exits with code `0` if all focused tests pass, `1` if any fail — same as normal mode.

### Testing focused behaviour in isolation

Because `.only()` is global, tests for focus mode must live in a **separate binary** compiled from their own `TEST_SUITE` file. This prevents focus mode from suppressing the rest of your test suite:

```cmake
add_executable(test_focus test_focus.cpp)
target_include_directories(test_focus PRIVATE include)
add_test(NAME focus_mode COMMAND test_focus)
```

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
    Sender Value;
public:
    explicit Notifier(Sender sender) : Value(std::move(sender)) {}
    void notify(std::string user, std::string msg) { Value(user, msg); }
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
- Does not work on Windows in this build. Windows support is on the roadmap but not implemented; the hotpatch module is **not part of the CI test surface** yet.
- Not thread-safe during the patch itself.
- No "call original" from the replacement: this is a replace-and-restore, not a full detour trampoline.

## Why two tiers?

The original design goal was "cross-platform" + "mock everything (even non-virtual)" + "no inheritance". These three constraints cannot all be satisfied *cleanly* at the same time. Tier A covers 95 % of real unit-testing needs with zero compromise. Tier B catches the remaining 5 % on platforms where hot-patching is reasonable, and fails explicitly elsewhere with a message telling you to use Tier A instead.

## Build

```bash
mkdir build && cd build
cmake .. && cmake --build .
./example
```

Or by hand:

```bash
g++ -std=c++17 -O0 -fno-inline -I include examples/example.cpp -o example
./example
```

## Contributing

Contributions are welcome. Please read this section before opening a pull request — it will save both of us a review cycle.

### Coding style

`cest` follows the [LLVM Coding Standards](https://llvm.org/docs/CodingStandards.html) with **one deliberate exception**: `iostream` is allowed. LLVM forbids it for binary-size and static-initialization-order reasons, but a test framework that prints coloured output to a terminal has no realistic alternative, and the `Runner` owns its stream lifetime so the traditional concerns don't apply here. Do not use `iostream` in code paths that aren't reporter output.

Everything else follows LLVM. The points that come up most often in review:

- **Names.** Types, classes, and variables use `PascalCase` (`HookCounters`, `BeforeAllOuter`, `Value`). Functions and methods use `camelCase` (`runSuite`, `mockReturnValue`). Macros are `CEST_UPPER_SNAKE_CASE`. Global variables are prefixed with `G` (`GHooks`). Member variables have no prefix — no `m_`, no trailing underscore.
- **`auto`.** Use `auto` only when the type is either obvious from the right-hand side (iterators, lambda returns, `make_*` factories) or genuinely too verbose to spell. `auto x = 42;` is wrong; `auto it = map.find(key);` is right.
- **Early returns.** Prefer them to nested `if`s. Reduce indentation; keep the happy path at the leftmost column.
- **No exceptions for control flow.** Exceptions are reserved for `AssertionError` and for errors the user should see at the top level. Do not use them to signal "not found" or similar expected conditions.
- **`const` everywhere it applies.** Parameters, locals, methods.
- **Include order.** Corresponding header first, then a blank line, then C++ standard library headers, then third-party, then project headers. Each group alphabetized.
- **Braces.** Attached, LLVM-style: `if (x) {`, not `if (x)\n{`.
- **Line length.** 80 columns. This is not negotiable for new code. Existing code that violates it can be reformatted in the same patch that touches it, but do not submit drive-by reformats.

When in doubt, read a neighbouring file and match it. If the neighbouring file is wrong, fix it in a dedicated commit, not bundled with a feature.

### Commits and pull requests

- **One logical change per PR.** "Fix hotpatch on ARM64 and also rename three variables and also add a matcher" is three PRs. Reviewers will ask you to split it.
- **Commit messages.** First line is a short imperative summary, 72 columns max, no trailing period. Blank line. Body explains *why*, not *what* — the diff shows what. Wrap at 72 columns.
    ```
    Fix beforeEach ordering in nested suites

    runSuite() was passing effectiveBefore to children instead of
    rebuilding the chain from inherited + s.BeforeEach. This caused
    the inner beforeEach to run twice on deeply nested suites.

    Fixes #42.
    ```
- **Link the issue.** If a PR fixes an open issue, say `Fixes #N` in the commit body so GitHub auto-closes it on merge.
- **Tests are required for behavioural changes.** A bug fix must come with a test that fails before the fix and passes after. A new matcher must come with tests for both the positive and the negated (`.Not()`) form. A new feature must come with at least one usage test in `examples/example.cpp`.
- **Documentation changes alongside code changes.** If you add a public API, update the README in the same PR. Documentation drift is worse than no documentation.
- **CI must be green before review.** PRs with a red CI will be ignored until they are green. If CI is flaky (not your fault), comment on the PR and re-run; don't silently push a merge commit.

### What to work on

The [ROADMAP.md](ROADMAP.md) lists everything that's on the table. If you want to tackle something, open an issue first saying *which* item and *how* you plan to approach it — this avoids two people working on the same feature in parallel. Small, isolated items (new matchers, docstring improvements, build-system cleanups) don't need a prior issue; just open the PR.

Things that will be rejected on sight, no matter how well-written:

- Mass reformats of existing code.
- Renames that don't serve a clear purpose.
- New dependencies. `cest` is header-only and self-contained. It will stay that way.
- Anything that adds a virtual base class or inheritance requirement to the mock module. The "no inheritance" constraint is load-bearing, not incidental.
- Features that duplicate what an external tool already does well (see ROADMAP section 5 — coverage reporting is CMake glue around `llvm-cov`, not a custom collector).

### Reporting bugs

Open an issue with:

1. Compiler, version, and OS (`g++ --version`, `clang++ --version`, `cl`).
2. The smallest `TEST_SUITE` that reproduces the problem. Ideally one that fits in a comment.
3. What you expected to see, and what you actually saw. Full output, not a paraphrase.
4. Whether you ran with `NO_COLOR=1` (ANSI escapes in a bug report are hard to read).

A bug report that's missing any of these will get a request for more information before anyone looks at the substance.