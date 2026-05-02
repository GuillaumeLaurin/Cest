# cest

A header-only C++20 test library inspired by Jest, with `describe` / `it` / `expect`, lifecycle hooks, and a two-tier mocking module — **without inheritance**.

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

Compile with `-std=c++20 -I include`, run the binary, and you get Jest-style coloured output.

## Matchers

## Custom matchers
 
`CEST_MATCHER` lets you define your own type-specific matchers once and reuse
them anywhere in your test suite with the same `expect(...).myMatcher()` syntax
as built-in matchers.
 
### Syntax
 
```cpp
CEST_MATCHER(Name, Type, Predicate, Description);
```
 
| Parameter     | What to pass                                                                 |
|---------------|------------------------------------------------------------------------------|
| `Name`        | The matcher method name (used as `expect(x).Name()`).                        |
| `Type`        | The exact C++ type the matcher targets (e.g. `int`, `std::string`).          |
| `Predicate`   | A callable `(const Type &) -> bool`. A lambda is the usual choice.           |
| `Description` | A string literal shown in the failure message as the "expected" value.       |
 
The macro must be placed at namespace scope, outside any function, `describe`,
or `TEST_SUITE` block. Include `<cest/core.hpp>` before using it.
 
### Basic example
 
```cpp
#include <cest/core.hpp>
#include <string>
 
// Define once — available in every TEST_SUITE in this translation unit.
CEST_MATCHER(isEven, int, [](int n) { return n % 2 == 0; }, "even number");
 
CEST_MATCHER(
    isPalindrome, std::string,
    [](const std::string &s) {
      std::string r(s.rbegin(), s.rend());
      return s == r;
    },
    "palindrome string");
 
TEST_SUITE("Custom matcher demo") {
  cest::describe("isEven", [] {
    cest::it("passes for an even integer",
             [] { cest::expect(4).isEven(); });
 
    cest::it("fails for an odd integer", [] {
      cest::expect(cest::Void([] { cest::expect(3).isEven(); })).toThrow();
    });
  });
 
  cest::describe("isPalindrome", [] {
    cest::it("passes for 'racecar'",
             [] { cest::expect(std::string("racecar")).isPalindrome(); });
    cest::it("fails for 'hello'", [] {
      cest::expect(cest::Void([] {
        cest::expect(std::string("hello")).isPalindrome();
      })).toThrow();
    });
  });
}
```
 
### Negation with `.Not()`
 
Custom matchers compose with `.Not()` exactly like built-in ones:
 
```cpp
CEST_MATCHER(isEven, int, [](int n) { return n % 2 == 0; }, "even number");
 
cest::expect(7).Not().isEven();   // passes — 7 is not even
 
// Negated failure:
cest::expect(cest::Void([] {
  cest::expect(2).Not().isEven(); // throws — 2 IS even
})).toThrow();
```
 
### Failure messages
 
When a custom matcher fails it throws a `cest::AssertionError` whose `what()`
string follows the same pattern as built-in matchers:
 
```
expect(<value>).<Name>(<Description>)       // positive failure
expect(<value>).not.<Name>(<Description>)   // negated failure
```
 
Example for `isEven` on value `3`:
 
```
expect(3).isEven(even number)
```
 
### Coexistence with built-in matchers
 
Registering a custom matcher for a type that already has built-in matchers (such
as `int` or `std::string`) does **not** shadow the built-ins. Both overloads
remain available:
 
```cpp
CEST_MATCHER(isEven, int, [](int n) { return n % 2 == 0; }, "even number");
 
cest::expect(42).isEven();   // custom matcher
cest::expect(42).toBe(42);   // built-in matcher — still works
```
 
C++ overload resolution picks the most-constrained `expect()` overload, so
there is no ambiguity.
 
### Sharing matchers across files
 
Because `CEST_MATCHER` defines a free `expect()` overload at namespace scope,
the cleanest way to share a matcher across multiple translation units is to put
its definition in its own header and `#include` that header wherever needed:
 
```
// matchers/is_even.hpp
#pragma once
#include <cest/core.hpp>
CEST_MATCHER(isEven, int, [](int n) { return n % 2 == 0; }, "even number");
```
 
Guard the header with `#pragma once` (or a traditional include guard) to prevent
multiple-definition errors when the header is included from more than one file in
the same translation unit.

### Equality

`toBe(expected)` — passes if `value == expected` (using `operator==`).

`toEqual(expected)` — alias for `toBe`; use when the intent is deep-value equality rather than identity.

`toStrictEqual(expected)` — passes if both the **type** and the **value** are identical. Unlike `toBe` and `toEqual`, this matcher fails immediately when `Actual` and `Expected` are different types (after stripping cv-qualifiers and references), even if the values would compare equal via `operator==`. When both types match, it performs a recursive `deepEqual` — the same element-wise comparison used internally by container matchers. For containers, the error message includes the index of the first mismatching element or a size-mismatch note.

```cpp
// Type match — value compared with deepEqual
expect(std::vector<int>{1, 2, 3}).toStrictEqual(std::vector<int>{1, 2, 3}); // passes
expect(std::vector<int>{1, 2, 3}).toStrictEqual(std::vector<int>{1, 2, 9}); // fails: mismatch at index 2

// Type mismatch — fails immediately, no value comparison
int i = 1;
long l = 1;
expect(i).Not().toStrictEqual(l); // passes: int != long

// Nested containers
std::vector<std::vector<int>> a = {{1, 2}, {3, 4}};
std::vector<std::vector<int>> b = {{1, 2}, {3, 4}};
expect(a).toStrictEqual(b); // passes

// Negation
expect(std::vector<int>{1, 2}).Not().toStrictEqual(std::vector<int>{1, 9}); // passes
```

> **Note:** The type check uses `std::is_same` after `std::remove_cv_t<std::remove_reference_t<T>>`. Two types that are identical after stripping qualifiers (e.g. `const int` and `int`) are considered the same type.

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
 
These matchers are available for any type that satisfies the internal `is_container` concept: it must expose `value_type`, `begin()`, `end()`, and `size()`. This covers `std::vector`, `std::array`, `std::map`, `std::set`, `std::string`, and similar types.
 
`toContain(needle)` — passes if any element in the container compares equal to `needle` using `operator==`.
 
For `std::map`, the element type is `std::pair<const K, V>`, so `needle` must be a pair:
 
```cpp
std::map<std::string, int> m = {{"a", 1}, {"b", 2}};
expect(m).toContain(std::pair<const std::string, int>{"b", 2});
```
 
For `std::string`, the element type is `char`, so `toContain` searches for a single character. Use `toMatch` for substring search.
 
`toContainEqual(expected)` — passes if any element in the container compares equal to `expected` using `deepEqual`, which recurses into nested containers. `Expected` must be convertible to the container's `value_type`.
 
```cpp
std::vector<std::vector<int>> v = {{1, 2}, {3, 4}};
expect(v).toContainEqual(std::vector<int>{3, 4});
```
 
`toHaveLength(n)` — passes if `value.size() == n`.
 
```cpp
expect(std::vector<int>{1, 2, 3}).toHaveLength(3);
expect(std::string("hello")).toHaveLength(5);
expect(std::set<int>{}).toHaveLength(0);
```
 
`toBeEmpty()` — passes if `value.empty()`.
 
```cpp
expect(std::vector<int>{}).toBeEmpty();
expect(std::map<int,int>{}).toBeEmpty();
expect(std::string{}).toBeEmpty();
```
 
All container matchers support `.Not()`:
 
```cpp
expect(std::vector<int>{1, 2, 3}).Not().toContain(99);
expect(std::vector<int>{1, 2, 3}).Not().toBeEmpty();
expect(std::vector<int>{1, 2, 3}).Not().toHaveLength(1);
```

### Partial object

`toMatchObject(partial)` — passes if the actual object satisfies all field
constraints declared in the `PartialType<T>` builder. Fields not mentioned in
the matcher are ignored entirely.

Use `PartialType<T>::field(ptr, value)` to declare each constraint. The first
argument is a member pointer (`&T::field`), the second is the expected value.

```cpp
struct User {
    std::string name;
    int         age;
    std::string email;
};

User user{"alice", 30, "alice@example.com"};

// Only name and age are checked — email is ignored
cest::expect(user).toMatchObject(
    cest::PartialType{}
        .field(&User::name, std::string("alice"))
        .field(&User::age, 30));

// Single field
cest::expect(user).toMatchObject(
    cest::PartialType{}.field(&User::age, 30));

// Empty matcher — vacuously passes for any object
cest::expect(user).toMatchObject(cest::PartialType{});
```

`.Not()` inverts the match — passes if **any** declared field fails to match:

```cpp
// Passes — age is 30, not 99
cest::expect(user).Not().toMatchObject(
    cest::PartialType{}.field(&User::age, 99));

// Fails — all fields match, so .Not() throws
cest::expect(cest::Void([&] {
    cest::expect(user).Not().toMatchObject(
        cest::PartialType{}.field(&User::age, 30));
})).toThrow();
```

On failure, the error message includes the matcher name, the expected value,
and the actual value of the mismatching field:

```
expect(<User>).toMatchObject( expected: 99
 received: 30)
```

**Key semantics:**

- Comparison is **asymmetric**: the actual object may have more fields than
  the matcher declares — only declared fields are checked.
- Fields are checked in declaration order; the first failure short-circuits
  the rest.
- Each field is compared with `operator==`.
- Member pointers (`&T::field`) replace string names — no reflection required,
  everything is resolved at compile time.

### Exceptions

These matchers are available when `expect` receives a `cest::Void` (i.e. a
`std::function<void()>`).

`toThrow()` — passes if the callable throws any exception.

`toThrowType<E>()` — passes if the callable throws an exception of type `E`
(caught by `catch (const E &)`).

`toThrowMessage(expected)` — passes if the callable throws a `std::exception`
whose `what()` string equals `expected`. Accepts `std::string` or `const char *`
(the `const char *` overload is removed; string literals are implicitly converted
to `std::string`).

```cpp
expect(cest::Void([] { throw std::runtime_error("boom"); })).toThrow();

expect(cest::Void([] { throw std::runtime_error("err"); }))
    .toThrowType();

expect(cest::Void([] { throw std::runtime_error("exact message"); }))
    .toThrowMessage("exact message");
```

All three matchers support `.Not()`:

```cpp
expect(cest::Void([] {})).Not().toThrow();

expect(cest::Void([] { throw std::logic_error("other"); }))
    .Not().toThrowType();

expect(cest::Void([] { throw std::runtime_error("actual"); }))
    .Not().toThrowMessage("different");
```

> **Note:** `toThrowMessage` only catches `std::exception` and its subclasses.
> A callable that throws an unrelated type (e.g. a raw `int`) will not be
> caught and the matcher will fail.

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
 
There are three paths — they solve different problems and trade portability for power.
 
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
 
This tier ships in the `Cest::Cest` (core) target — no extra link required.
 
### Tier B: `hotpatch()` — runtime interception
 
For cases where you **cannot** modify the calling code: free functions from a third-party library, existing non-virtual methods that you do not want to templatize. Rewrites the first instructions of the target function with a jump to your replacement, and restores on guard destruction.
 
Supported on Linux, macOS, and Windows, on x86_64 and ARM64.
 
```cpp
int real_multiply(int a, int b) { return a * b; }
int fake_multiply(int, int) { return 999; }
 
{
    auto guard = hotpatch(&real_multiply, &fake_multiply);
    real_multiply(6, 7);  // -> 999
}
real_multiply(6, 7);      // -> 42 (restored)
```
 
Hot-patching has hard requirements that conflict with several modern compiler defaults (Control Flow Guard, Incremental Linking, function inlining). To keep the user from having to discover and apply each flag manually, the library exposes two CMake facets: `Cest::Mock` and `Cest::MockStrict`.
 
#### `Cest::Mock` — recommended default
 
Link `Cest::Mock` and you get hot-patching that just works. No annotations on your functions are required.
 
```cmake
find_package(Cest REQUIRED)
add_executable(my_tests test_main.cpp)
target_link_libraries(my_tests PRIVATE Cest::Mock)
```
 
Linking `Cest::Mock` propagates these flags to the consumer:
 
- `/guard:cf-` and `/GUARD:NO` (MSVC) — disable Control Flow Guard.
- `/INCREMENTAL:NO` (MSVC) — disable incremental linking, which would otherwise place a thunk before each function and break the patch.
- `/Ob0` (MSVC) or `-fno-inline -fno-inline-functions` (GCC/Clang) — disable inlining globally so that any function can be hot-patched without explicit annotation.
 
Trade-off: the test binary loses inlining, so it's a less optimized build than production. For a unit-test binary this is almost always acceptable.
 
> **Note (MSVC):** when `STRICT_MODE` is on, MSVC will emit warning D9025 ("overriding `/guard:cf` with `/guard:cf-`") at compile time. This is intentional — Control Flow Guard must be disabled for hot-patching to work. The warning is cosmetic and does not fail the build. `D9025` is a command-line driver warning and cannot be suppressed via `/wd9025`.
 
#### `Cest::MockStrict` — fine-grained control
 
Link `Cest::MockStrict` if you want to keep inlining enabled in your test binary (e.g. for performance-sensitive integration tests). In exchange, **every function you intend to hot-patch must be annotated** with `CEST_MOCKABLE` (which expands to `__declspec(noinline)` on MSVC and `__attribute__((noinline))` elsewhere).
 
```cpp
#include <cest/macros/attributes.hpp>
 
CEST_MOCKABLE int real_multiply(int a, int b) { return a * b; }
 
// In test:
auto guard = hotpatch(&real_multiply, &fake_multiply);
```
 
`Cest::MockStrict` propagates the same Control Flow Guard and incremental linking flags as `Cest::Mock`, but does **not** disable inlining globally.
 
#### `hotpatchMethod()` — non-virtual member functions
 
For non-virtual methods, use the `hotpatchMethod` overload. The replacement is a free function whose first parameter is the class pointer (the implicit `this`):
 
```cpp
struct Calculator {
    int compute(int x) { return x * 10; }
};
 
int mockCompute(Calculator*, int) { return -1; }
 
Calculator c;
auto guard = hotpatchMethod(&Calculator::compute, &mockCompute);
c.compute(5);  // -> -1
```
 
A const-qualified overload is provided for `const` methods. The replacement's first parameter must then be `const Class*`.
 
#### Caveats to know about
 
- **Affects all instances.** `hotpatchMethod` patches the function code, which is shared across all instances of the class. Per-instance mocking of virtual methods is on the roadmap (vtable swap).
- **Not for virtual methods.** Use `hotpatchMethod` only for non-virtual methods. Virtual dispatch goes through the vtable, which `hotpatch` does not touch.
- **Multiple/virtual inheritance under MSVC.** `hotpatchMethod` extracts the function address from a member pointer via `memcpy`. Under MSVC, this is reliable only for single-inheritance classes. Classes with multiple or virtual inheritance use a larger member-pointer representation.
- **Not thread-safe during the patch itself.** Apply patches in single-threaded test setup, not concurrently.
- **No "call original" from the replacement.** This is replace-and-restore, not a full detour trampoline.
- **Build configuration:** when building Cest from source rather than via `find_package`, set `-DSTRICT_MODE=OFF` if you don't want the test binaries themselves to inherit `/guard:cf` from `CommonConfig`.
 
### Why three tiers?
 
The original design goal was "cross-platform" + "mock everything (even non-virtual)" + "no inheritance". These three constraints cannot all be satisfied *cleanly* at the same time. Tier A (`Cest::Mock`) covers 95% of real unit-testing needs with zero compromise. Tier B (`Cest::Mock`) catches the remaining 5% with zero annotation cost in exchange for losing inlining. Tier C (`Cest::MockStrict`) gives you the same hot-patching power while keeping your test binary fully optimized, at the cost of one annotation per hot-patchable function.
````

## Build

```bash
mkdir build && cd build
cmake .. && cmake --build .
./example
```

Or by hand:

```bash
g++ -std=c++20 -O0 -fno-inline -I include examples/example.cpp -o example
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
- **Tests are required for behavioural changes.** A bug fix must come with a test that fails before the fix and passes after. A new matcher must come with tests for both the positive and the negated (`.Not()`) form. A new feature must come with at least one usage test in `examples/example.cpp`. A new container matcher must be tested against at least `std::vector`, `std::string`, and one associative container (`std::map` or `std::set`). Document any type-specific caveat (e.g. `std::map` requires a pair, `std::string` element type is `char`) in both the test file and the README.
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