# Cest — Roadmap

A pragmatic plan for turning Cest from a working Jest-style test library into something you'd actually reach for on a real project. Items are grouped by theme, not strict priority — pick whichever unblocks your current pain point first.

## Legend

- 🟢 **Small** — Small effort.
- 🟡 **Medium** — Medium effort.
- 🔴 **Large** — Enormous effort.
- ✅ **Done** — Implemented and shipped.

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

### 1.2 ✅ `it.skip` / `it.only` and `describe.skip` / `describe.only`

> **Done.** Shipped via #3 (`it.skip` / `describe.skip`) and #4 (`it.only` / `describe.only`). See the README sections "Skipping tests and suites" and "Focusing tests and suites" for the user-facing docs.

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

### 2.1 ✅ Full Jest matcher parity

> **Done.** Shipped across multiple PRs:
>
> - **Equality and identity**: `toStrictEqual` (#6), `toMatchObject` (#7).
> - **Numbers**: `toBeCloseTo`, `toBeNaN`, `toBeFinite`, `toBeInfinite`, `toBeGreaterThanOrEqual`, `toBeLessThanOrEqual` (#8).
> - **Strings**: `toMatch`, `toStartWith`, `toEndWith` (#9).
> - **Containers**: `toHaveLength` (#10), `toContainEqual`, `toBeEmpty` (#11).
> - **Exceptions**: `toThrowMessage` (#12); `toThrowType<E>()` was already present and is now documented.
>
> All new matchers compose with `.Not()`. See the README "Matchers" section for the full reference.

### 2.2 ✅ Custom matchers

> **Done.** Shipped via #13. Users can register their own matchers with the `CEST_MATCHER(Name, Type, Predicate, Description)` macro. See the README "Custom matchers" section.

---

## 3. Mocking enhancements

### 3.1 ✅ Hot-patch on Windows

> **Done.** Windows is now a first-class target for `cest::hotpatch`, alongside Linux and macOS, on both x86_64 and ARM64. The implementation:
>
> - Uses `VirtualProtect` (`PAGE_EXECUTE_READWRITE` during the patch, restored to `PAGE_EXECUTE_READ` after).
> - Reuses the same x86_64 (`movabs rax, imm64; jmp rax`) and AArch64 (`ldr x16, #8; br x16; <addr>`) encodings as the POSIX path.
> - Calls `FlushInstructionCache(GetCurrentProcess(), addr, size)` unconditionally on Windows.
> - Documents and enforces the build-time requirements (`/INCREMENTAL:NO`, `/guard:cf-` / `/GUARD:NO`, inlining control) via the new `Cest::Mock` and `Cest::MockStrict` CMake facets — users get the right flags automatically by linking the appropriate target.
>
> Two facets are exported so users can choose their trade-off:
>
> - `Cest::Mock` — disables inlining globally (`/Ob0` / `-fno-inline`) so no annotations are required on hot-patchable functions.
> - `Cest::MockStrict` — keeps inlining; users must annotate hot-patchable functions with `CEST_MOCKABLE`.
>
> A `hotpatchMethod()` overload was also added for non-virtual member functions. See the README "Tier B: `hotpatch()`" section for the full user-facing docs.

### 3.2 🟢 Per-instance mocking of virtual methods (`VTableGuard`)

Hot-patching today rewrites function code, which is shared across all instances of a class. For virtual methods, that's the wrong granularity — most tests want to swap behavior on a single instance without affecting siblings.

- `VTableGuard::forInstance(instance, &Class::method, &replacement)` — copies the original vtable into a heap buffer, modifies one slot in the copy, and points the instance's vptr at the copy. Other instances are unaffected. Restores the vptr on guard destruction.
- `VTableGuard::forClass(any_instance, &Class::method, &replacement)` — modifies the actual class vtable in place. Affects all instances. Use sparingly; useful for global stubs.
- Slot index extraction works automatically on Itanium ABI (Linux, macOS — GCC and Clang). On MSVC, the slot index must be passed explicitly via `forInstanceSlot(instance, slot, replacement)` because MSVC's pointer-to-virtual-member is a generated thunk, not an encoded index.

**Caveats to document:** the guard must outlive the instance (otherwise the vptr points into freed memory); single-vptr layout is assumed (multiple inheritance with multiple sub-objects requires adjusting the instance pointer to the right sub-object); the swapped vtable is a copy, so RTTI on the patched instance still returns the original `type_info`.

**Implementation notes.** This is a separate tool from `hotpatch` — it doesn't touch executable memory at all, just data. No `VirtualProtect` / `mprotect` needed for the per-instance variant; the class-wide variant does need write access to the vtable section.

### 3.3 🟡 Spy on existing callables

`MockFn` today is a standalone mock. Add a `spyOn` equivalent that wraps an existing `std::function` or lambda, forwards calls to it, *and* records them.

```cpp
auto real = [](int a, int b) { return a + b; };
auto spied = spy(real);
spied(1, 2);  // returns 3, also records the call
expect(spied).toHaveBeenCalledWith(1, 2);
```

Useful when the production code passes you a functor and you just want to observe it without reimplementing behavior.

### 3.4 🟢 Call-order assertions across mocks

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

With the matcher parity (#2.1), focus/skip (#1.2), custom matchers (#2.2), and Windows hot-patch (#3.1) all done, the next priorities are:

1. **1.3 CLI filtering** — once the suite has a few dozen tests it becomes painful to run only one. The skip/focus plumbing from 1.2 already exists; filtering is a thin layer on top.
2. **4.1 JUnit reporter** (plus **4.2 timing** as a prerequisite) — unlocks CI integration.
3. **5.0 Code coverage** — pairs naturally with the JUnit reporter; same CI run produces both artifacts. Do it right after 4.1 while you're still in "CI wiring" mode.
4. **3.2 VTable-based mocking** — small effort, completes the mocking story for virtual methods. Natural follow-up to the hot-patch work.
5. **1.1 Randomization** — valuable but can wait until the suite is big enough that ordering bugs become plausible.
6. **3.3 / 3.4 Mock enhancements** — nice to have, rarely blocking.
7. **6.x Ergonomics** — polish, do last.