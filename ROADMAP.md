# Cest — Roadmap

A pragmatic plan for turning Cest from a working Jest-style test library into something you'd actually reach for on a real project. Items are grouped by theme, not strict priority — pick whichever unblocks your current pain point first.

## Legend

- 🟢 **Small** — Small effort.
- 🟡 **Medium** — Medium effort.
- 🔴 **Large** — Enormous effort.

---

## 1. Test execution control

### 1.1 🟡 Test randomization

Shuffle test and hook order to surface hidden ordering dependencies, the way Jest does with `--randomize` and RSpec does by default.

- Randomize the order of `it` blocks within each `describe`.
- Randomize the order of sibling `describe` blocks.
- **Do not** randomize `beforeAll` / `afterAll` / `beforeEach` / `afterEach` relative to the tests they wrap — their ordering is semantic, not incidental. But when a suite registers multiple hooks of the same kind, their relative order *can* be shuffled to catch suites that rely on registration order.
- Seed must be printable and reproducible: `--seed=12345` on the CLI, and the runner prints `Random seed: 12345` at the top of every run so a failing CI build can be reproduced locally.
- Default: randomized. Opt out with `--no-random` for debugging.

**Implementation notes.** `std::shuffle` with `std::mt19937` seeded from `std::random_device` (or the CLI flag). Shuffling happens in `Runner::run()` just before `runSuite` is first called — walk the tree once and shuffle in place on mutable copies.

### 1.2 🟢 `it.skip` / `it.only` and `describe.skip` / `describe.only`

Jest-style focus and skip. Essential for iterating on a single failing test without commenting out the rest of the file.

- `it.skip("name", []{...})` — registers the test but marks it skipped; reported as `○ name (skipped)` in yellow.
- `it.only("name", []{...})` — if *any* `only` exists in the run, *only* those tests (and their ancestor hooks) execute. Everything else is reported as skipped.
- Same for `describe.skip` / `describe.only`.
- C++ syntax note: `it.skip` is not directly possible (dot on a free function) — expose as a struct with overloaded `operator()` plus `.skip` / `.only` members, so the call site reads `it("name", ...)`, `it.skip("name", ...)`, `it.only("name", ...)`. Same trick for `describe`.

**Implementation notes.** Add `bool skipped` and `bool focused` to `TestCase` and `Suite`. Two-pass runner: first pass collects whether any `.only` exists anywhere in the tree; second pass executes, skipping anything not focused if the flag is set.

### 1.3 🟡 CLI filtering by name

Run only tests matching a pattern — the single most-used feature in any serious test runner.

```
./cest_example --filter "MockFn"
./cest_example --filter "addition/negative numbers"
./cest_example -t "should.*throw"   # regex mode
```

- Plain substring match by default, against the full `describe > describe > it` path.
- `--regex` / `-e` flag to switch to `std::regex`.
- Combine with `.only` semantics: a filter hit implicitly focuses.
- Print the number of filtered-out tests at the end: `Filtered: 47 tests excluded by --filter`.

**Implementation notes.** Compute each test's full path as a string during tree traversal, match against the pattern, skip non-matches. The existing skip/focus plumbing from 1.2 does the heavy lifting — filtering is just "auto-skip everything that doesn't match".

---

## 2. Matchers

### 2.1 🟡 Full Jest matcher parity

Current set is the minimum viable. Jest users expect these without thinking. Prioritize the ones people reach for daily:

**Equality and identity**
- `toStrictEqual` — deep equality that also checks types (for containers and aggregates)
- `toMatchObject` — partial object match against a struct with designated initializers or a `std::map`-like subset

**Numbers**
- `toBeCloseTo(value, precision)` — floating-point comparison with configurable precision
- `toBeNaN`, `toBeFinite`, `toBeInfinite`
- `toBeGreaterThanOrEqual`, `toBeLessThanOrEqual`

**Strings**
- `toMatch(regex_or_substring)` — regex or substring match
- `toStartWith`, `toEndWith`
- `toHaveLength(n)` — also works on containers

**Containers**
- `toHaveLength(n)`
- `toContainEqual` — like `toContain` but uses deep equality instead of `==`
- `toBeEmpty`

**Exceptions**
- `toThrowType<E>()` — already present, document it
- `toThrowMessage("substring")` — catches any exception derived from `std::exception` and matches against `what()`

**Negation**
- Make sure every new matcher composes with `.Not()`.

**Implementation notes for `toMatchObject`.** The tricky one. C++ has no reflection, so "partial object match" needs a helper type — probably a `PartialMatch` builder:
```cpp
expect(user).toMatchObject(PartialMatch{}
    .field("name", std::string("alice"))
    .field("age", 30));
```
Less ergonomic than JS but honest about the language. Alternatively, for types the user controls, a `CEST_REFLECT(Type, field1, field2, ...)` macro that generates a comparator — more setup, much better syntax.

### 2.2 🟢 Custom matchers

Let users register their own matchers once and reuse them across the codebase.

```cpp
CEST_MATCHER(toBeEven, int, [](int n) {
    return n % 2 == 0;
}, "even number");
```

Minimal surface: a macro that generates a free function returning a matcher object, hooked into the `Expectation` template via ADL or a trait.

---

## 3. Mocking enhancements

### 3.1 🔴 Hot-patch on Windows

Currently the POSIX-only path throws an explicit error on Windows. Making it work requires:

- **Memory protection.** Replace `mprotect` with `VirtualProtect` (`PAGE_EXECUTE_READWRITE` during the patch, restore to `PAGE_EXECUTE_READ` after).
- **Instruction encoding.** x86_64 encoding is identical to Linux (`movabs rax, imm64; jmp rax`), so the 12-byte sequence in `hotpatch.hpp` ports directly. ARM64 Windows uses the same AArch64 encoding as Linux — no change needed.
- **Incremental Linking (`/INCREMENTAL`).** MSVC Debug builds insert a jump thunk at every function entry (`jmp target`) so the linker can patch functions in place. You end up patching the *thunk*, not the real function — which actually still works, but the semantics are subtler. Recommend building tests with `/INCREMENTAL:NO` and document it loudly.
- **ASLR and `/guard:cf`.** Control Flow Guard rejects jumps to non-registered targets. Document how to disable for test builds (`/guard:cf-`).
- **I-cache flush.** `FlushInstructionCache(GetCurrentProcess(), addr, size)` — there's no coherent-x86 shortcut; call it unconditionally on Windows.

**Don't pretend it's cross-platform until it's tested on Windows.** Set up CI with `windows-latest` + MSVC and run the hot-patch test before claiming support.

### 3.2 🟡 Spy on existing callables

`MockFn` today is a standalone mock. Add a `spyOn` equivalent that wraps an existing `std::function` or lambda, forwards calls to it, *and* records them.

```cpp
auto real = [](int a, int b) { return a + b; };
auto spied = spy(real);
spied(1, 2);  // returns 3, also records the call
expect(spied).toHaveBeenCalledWith(1, 2);
```

Useful when the production code passes you a functor and you just want to observe it without reimplementing behavior.

### 3.3 🟢 Call-order assertions across mocks

Jest has `mock.calls` per mock but lacks cross-mock ordering out of the box. Add a global call log (opt-in) so users can assert "`sendEmail` was called before `logAudit`".

```cpp
CallOrder order;
MockFn<void(std::string)> sendEmail{order};
MockFn<void(std::string)> logAudit{order};
// ... system under test runs ...
expect(order).toHaveCalledInOrder(sendEmail, logAudit);
```

---

## 4. Reporting and CI

### 4.1 🟡 JUnit XML reporter

The universal CI format — every CI system (GitHub Actions, GitLab, Jenkins, CircleCI, Azure DevOps) knows how to render JUnit XML test reports and surface failures inline on pull requests.

- `--reporter=junit --output=results.xml`
- Support multiple reporters simultaneously: `--reporter=console --reporter=junit:results.xml`
- Include per-test duration (requires wiring `std::chrono` timing into `runSuite`).
- Map nested `describe` paths to JUnit's `<testsuite>` hierarchy. JUnit only nominally supports nesting — most consumers flatten it, so emit a flat list with dotted names: `"lifecycle hooks.nested suite.wraps outer hooks"`.
- Escape XML properly in test names and error messages (`&`, `<`, `>`, `"`, `'`).

**Implementation notes.** Abstract the current console output behind a `Reporter` interface with `onSuiteStart`, `onTestPass`, `onTestFail`, `onRunEnd`. Current coloured output becomes `ConsoleReporter`; `JUnitReporter` writes XML on `onRunEnd`. Low risk if done before the other features churn the runner.

### 4.2 🟢 Per-test timing

A prerequisite for 4.1 but useful on its own. Print `(12ms)` after each test name; flag slow tests in yellow above a threshold (`--slow-threshold=100ms`).

### 4.3 🟢 Exit codes and summary line

Current behavior is fine (`0` = all passed, `1` = failures), but add:

- Distinct exit code for `no tests matched the filter` (commonly `2`) so CI can distinguish "all green" from "nothing ran".
- Machine-readable summary line when `--summary-only` is passed: `PASS 42 FAIL 0 SKIP 3 DURATION 1.4s`.

---

## 5. Code coverage

### 5.0 🔴 Unit test coverage reporting

Know which lines, branches, and functions are actually exercised by the test suite. Essential for any codebase that wants to claim it's tested — and a natural CI artifact alongside the JUnit reporter from 4.1.

**Don't reinvent the instrumentation.** Modern compilers already emit coverage data; Cest's job is to integrate cleanly with that pipeline, not to build a parallel one.

- **Clang / GCC**: `-fprofile-instr-generate -fcoverage-mapping` (Clang) or `--coverage` (GCC). Running the test binary drops a `.profraw` / `.gcda` file per translation unit.
- **MSVC**: native coverage via `/fsanitize-coverage=inline-8bit-counters` or the Visual Studio `VSInstr` tooling — more painful, usually easier to route Windows CI through Clang-CL.
- **Post-processing**: `llvm-cov` or `lcov` + `genhtml` produces human-readable HTML reports; `llvm-cov export -format=lcov` or `gcovr --cobertura` produces the XML formats that Codecov, Coveralls, and SonarQube consume.

**What Cest should provide:**

- A `cmake` option `CEST_ENABLE_COVERAGE=ON` that adds the right compile/link flags for the detected compiler. One-liner for users, same behavior across Clang and GCC.
- A `cest_add_coverage_target(target_name)` CMake helper that wires up a `coverage` custom target running the tests and producing `coverage.lcov` + an HTML report in `build/coverage/`.
- **Exclude the test files themselves and the Cest headers from coverage.** Otherwise the report gets polluted by 100%-covered framework code and fixture boilerplate, which drowns out the actual signal. Use `lcov --remove` patterns or `llvm-cov`'s `-ignore-filename-regex`.
- Document the minimum meaningful metric: **line coverage is a floor, not a target**. Branch coverage catches more bugs; mutation testing (a future item, not on this roadmap) catches even more. A suite at 95% line coverage with no branch coverage is lying to you.

**What Cest should NOT do.** Don't embed a coverage collector inside the framework. Don't try to parse `.gcov` output and print per-test coverage deltas. Don't build a custom HTML reporter. Every one of those has a mature external tool; Cest's value is the ergonomic CMake glue, nothing more.

**CI integration.** Once 4.1 (JUnit) and 5.0 are both done, a typical GitHub Actions job looks like:

```yaml
- run: cmake -B build -DCEST_ENABLE_COVERAGE=ON
- run: cmake --build build
- run: ./build/cest_example --reporter=junit:results.xml
- run: cmake --build build --target coverage
- uses: codecov/codecov-action@v4
  with: { file: build/coverage/coverage.lcov }
```

That's the whole user experience. If anything is more complicated than that, the integration is wrong.

---

## 6. Ergonomics

### 6.1 🟢 Better assertion diffs

Currently `expect(2).toBe(1)` prints exactly that. Fine for primitives, unhelpful for containers and strings. Add:

- Multi-line diff for strings longer than ~40 characters.
- Element-wise diff for vectors and maps (show the first mismatching index).
- Truncate gigantic values with a `… (123 more)` marker.

### 6.2 🟡 Fixture helper

Not Jest-style exactly, but so common it deserves syntactic sugar. A `Fixture<T>` that calls `beforeEach` to construct and `afterEach` to destroy:

```cpp
describe("Database", [] {
    auto db = fixture<Database>([]{ return Database::temp(); });
    it("stores a row", [&] {
        db->insert({1, "hello"});
        expect(db->count()).toBe(1);
    });
});
```

Cleaner than a captured `shared_ptr` plus manual reset in `beforeEach`.

### 6.3 🟢 Stable `TEST_SUITE` names

Today the macro uses `__LINE__` to avoid symbol collisions. It works but produces ugly internal names and breaks if two tests land on the same line after a refactor. Take the `Name` argument and build a stable identifier from it (with line as a tiebreaker).

---

## Ordering

1. **1.3 CLI filtering** and **1.2 skip/only** — you'll want these the moment the test suite grows past a few dozen cases.
2. **4.1 JUnit reporter** (plus **4.2 timing** as a prerequisite) — unlocks CI integration.
3. **5.0 Code coverage** — pairs naturally with the JUnit reporter; same CI run produces both artifacts. Do it right after 4.1 while you're still in "CI wiring" mode.
4. **2.1 Matcher parity** — incremental, can be done one matcher at a time between other work.
5. **1.1 Randomization** — valuable but can wait until the suite is big enough that ordering bugs become plausible.
6. **3.1 Windows hot-patch** — highest effort, lowest urgency unless you actually need it. Tier A (`MockFn`) covers most real cases.
7. **3.2 / 3.3 Mock enhancements** — nice to have, rarely blocking.
8. **6.x Ergonomics** — polish, do last.