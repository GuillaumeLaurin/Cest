<!--
Thanks for contributing to cest!

Before you open this PR, please make sure of the following:

  1. Read CONTRIBUTING section in README.md. It describes the coding style
     (LLVM, with iostream allowed), how commits should be structured, and
     what will be rejected on sight.

  2. Do not modify `include/cest.hpp` umbrella header or any version/release
     files as part of a feature PR. Those are touched only when a new
     release is cut.

  3. Make sure CI is green on your branch before requesting review. Red CI
     PRs will not be looked at. If a job is flaky and not your fault, say
     so in a comment and re-run it — do not push an empty commit to hide it.

  4. One logical change per PR. A bug fix is one PR. A new matcher is
     another PR. A rename is a third PR. Reviewers will ask you to split
     anything bundled.

The title of this PR should be a short imperative sentence, no trailing
period, 72 columns max. Examples:

  Good:  Fix beforeEach ordering in nested suites
  Good:  Add toBeCloseTo matcher for floating-point comparison
  Bad:   Fixed a bug  (too vague, past tense)
  Bad:   updates      (tells the reviewer nothing)
-->

## Description

<!--
Describe the *what* and the *why* of your pull request. These are usually
two different things.

  - What: the concrete change. "Rewrote runSuite to pass rebuilt hook chains
    to children instead of reusing the parent's effectiveBefore list."
  - Why:  the motivation. "beforeEach was running twice in deeply nested
    describes, which broke the trace-order assertion in HooksDemo."

If the why is a bug, describe how to reproduce it *before* your fix, and
what the expected behaviour is *after*. A reviewer should be able to read
this section and understand the problem without running the code.
-->

## Kind of change

<!-- Keep the ones that apply, delete the rest. -->

- [ ] Bug fix (non-breaking change that fixes an issue)
- [ ] New feature (non-breaking change that adds functionality)
- [ ] Breaking change (fix or feature that would cause existing code to
      behave differently)
- [ ] Documentation only (README, ROADMAP, comments, contributing docs)
- [ ] Build / CI / tooling (CMake, GitHub Actions, repo configuration)

## GitHub issues

<!--
If this PR was motivated by an existing issue, reference it here.

  - For a straightforward bug fix, add `Closes #123` to the commit message
    itself (not just this PR description) so GitHub auto-closes the issue
    on merge.
  - For a feature that may need several iterations or design discussion,
    do NOT use `Closes` — reference the issue with `Related to #123` and
    let the maintainer decide when to close it.
  - If there is no issue, that is fine for small, self-contained changes.
    For anything larger, please open an issue first and link it here so
    the approach can be discussed before you write a lot of code.
-->

## Tests

<!--
Every behavioural change must come with a test. Describe what you added
and why it covers the change.

  - For a bug fix: a test that fails on `main` and passes on your branch.
  - For a new matcher: at least one positive case and one `.Not()` case.
  - For a new feature: a usage example in `examples/example.cpp` exercising
    the feature end-to-end.

If the change is documentation-only or purely build-system, write "N/A"
and briefly explain why no test is needed.
-->

## Checklist

- [ ] I read the Contributing section of `README.md`.
- [ ] My code follows the LLVM style rules described there (`PascalCase`
      for types and variables, `camelCase` for functions, `G` prefix for
      globals, 80-column lines, attached braces).
- [ ] I added tests that cover my change (or explained above why none
      were needed).
- [ ] I updated documentation where the public API changed.
- [ ] My commit messages follow the format described in Contributing
      (imperative summary ≤72 cols, blank line, body explaining *why*).
- [ ] CI is green on this branch.
- [ ] This PR contains a single logical change.
- [ ] I have NOT modified `include/cest.hpp` or any release/version files
      unless this PR is explicitly a release.