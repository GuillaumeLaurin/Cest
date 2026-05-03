#ifndef CONSOLE_REPORTER_HPP
#define CONSOLE_REPORTER_HPP

#include "cest/reporters/i_reporter.hpp"

#include "cest/utils/colors.hpp"

#include "cest/utils/suite.hpp"

#include "cest/utils/test_case.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef CEST_USE_TABULATE
#include <tabulate/table.hpp>
#endif

namespace cest {

namespace reporters {

/**
 * @brief Console reporter that mimics Jest's output as closely as possible.
 *
 * Layout per file:
 *
 *   PASS  ./path/to/file.cpp
 *     <suite name>
 *       ✓ test 1 (3 ms)
 *       ✓ test 2
 *       ✗ failing test
 *       ○ skipped test
 *
 * Plus a final aggregated summary block:
 *
 *   Test Suites: 1 passed, 1 total
 *   Tests:       4 passed, 4 total
 *   Snapshots:   0 total
 *   Time:        0.436 s
 *   Ran all test suites.
 */
class ConsoleReporter : public IReporter {
public:
  /**
   * @param useTable If true, the final summary block is rendered as a tabulate
   *                 table with borders. Otherwise it uses Jest's plain
   *                 column-aligned format.
   */
  explicit ConsoleReporter(bool useTable = false) : UseTable(useTable) {}

  ~ConsoleReporter() override = default;

  void onFocussed() override {
    std::cout << "\n"
              << utils::italic() << utils::gray()
              << "Focus mode activated. Do not forget to turn it off"
              << utils::reset() << "\n";
  }

  void onRunStart(uint64_t /*seed*/, bool /*passed*/) override {
    StartTime = std::chrono::steady_clock::now();
    FilesPassed = FilesFailed = 0;
  }

  void onSuiteStart(const utils::Suite &suite, int depth) override {
    if (suite.IsTestSuiteRoot) {
      // Start buffering output for this file.
      Buffer.str("");
      Buffer.clear();
      CurrentFile = suite.File;
      CurrentFileFailed = 0;
      CurrentFilePassed = 0;
      CurrentFileSkipped = 0;
      InTestSuiteFile = true;
      DescribeDepth = 0;
      return;
    }

    if (!suite.Name.empty()) {
      // The test-suite-root wrapper is transparent, so its children are
      // displayed at depth 0 (one indent past the banner).
      indent(out(), DescribeDepth + 1);
      if (suite.Skipped) {
        out() << utils::bold() << utils::yellow() << suite.Name
              << utils::reset() << "\n";
      } else {
        out() << utils::bold() << suite.Name << utils::reset() << "\n";
      }
      ++DescribeDepth;
      ++NamedDescribeOpen;
    }
    (void)depth;
  }

  void onSuiteEnd(const utils::Suite &suite, int /*depth*/) override {
    if (suite.IsTestSuiteRoot) {
      // Print the PASS/FAIL banner first, then flush the buffered detail.
      bool failed = CurrentFileFailed > 0;
      if (failed) {
        std::cout << utils::bold() << utils::black() << utils::bgRed()
                  << " FAIL " << utils::reset() << " " << utils::bold()
                  << CurrentFile << utils::reset() << "\n";
        ++FilesFailed;
      } else {
        std::cout << utils::bold() << utils::black() << utils::bgGreen()
                  << " PASS " << utils::reset() << " " << utils::bold()
                  << CurrentFile << utils::reset() << "\n";
        ++FilesPassed;
      }
      std::cout << Buffer.str();

      InTestSuiteFile = false;
      DescribeDepth = 0;
      return;
    }

    if (!suite.Name.empty()) {
      --DescribeDepth;
      --NamedDescribeOpen;
    }
  }

  void onTestPass(const utils::TestCase &test,
                  std::chrono::nanoseconds duration) override {
    indent(out(), DescribeDepth + 1);
    out() << utils::green() << "\xE2\x9C\x93 " << utils::reset() << test.Name;
    appendDuration(out(), duration);
    out() << "\n";
    ++CurrentFilePassed;
  }

  void onTestFail(const utils::TestCase &test, const std::string &err,
                  std::chrono::nanoseconds duration) override {
    indent(out(), DescribeDepth + 1);
    out() << utils::red() << "\xE2\x9C\x97 " << utils::reset() << test.Name;
    appendDuration(out(), duration);
    out() << "\n";
    indent(out(), DescribeDepth + 2);
    out() << utils::red() << err << utils::reset() << "\n";

    Failures.push_back({CurrentFile, test.Name, err});
    ++CurrentFileFailed;
  }

  void onTestSkip(const utils::TestCase &test) override {
    indent(out(), DescribeDepth + 1);
    out() << utils::yellow() << "\xE2\x97\x8B " << utils::reset()
          << utils::gray() << "skipped " << test.Name << utils::reset() << "\n";
    ++CurrentFileSkipped;
  }

  void onHookError(const utils::Suite &suite, const std::string &hookName,
                   const std::string &message) override {
    indent(out(), DescribeDepth + 1);
    out() << utils::red() << hookName << " threw"
          << (suite.Name.empty() ? "" : " in \"" + suite.Name + "\"") << ": "
          << message << utils::reset() << "\n";
    HookErrors.push_back({CurrentFile, suite.Name, hookName, message});
    ++CurrentFileFailed; // Treat hook errors as failures for the banner.
  }

  void onRunEnd(int passed, int failed, int skipped) override {
    auto end = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - StartTime)
            .count();

    // Failure recap (Jest-style "● Test Name" header per failure).
    if (!Failures.empty() || !HookErrors.empty()) {
      std::cout << "\n"
                << utils::bold() << "Summary of all failing tests"
                << utils::reset() << "\n";
      for (const auto &f : Failures) {
        std::cout << "\n"
                  << utils::red() << utils::bold() << "  \xE2\x97\x8F "
                  << f.File << " > " << f.TestName << utils::reset() << "\n";
        std::cout << "    " << utils::red() << f.Message << utils::reset()
                  << "\n";
      }
      for (const auto &h : HookErrors) {
        std::cout << "\n"
                  << utils::red() << utils::bold() << "  \xE2\x97\x8F "
                  << h.File << " > " << h.HookName
                  << (h.SuiteName.empty() ? "" : " in \"" + h.SuiteName + "\"")
                  << utils::reset() << "\n";
        std::cout << "    " << utils::red() << h.Message << utils::reset()
                  << "\n";
      }
    }

    std::cout << "\n";

    if (UseTable) {
      printSummaryTable(passed, failed, skipped, elapsed);
    } else {
      printSummaryJestStyle(passed, failed, skipped, elapsed);
    }

    std::cout << "Ran all test suites." << std::endl;
  }

private:
  struct FailureRecord {
    std::string File;
    std::string TestName;
    std::string Message;
  };

  struct HookErrorRecord {
    std::string File;
    std::string SuiteName;
    std::string HookName;
    std::string Message;
  };

  bool UseTable = false;

  // Buffered output for the current file (so PASS/FAIL banner can be
  // printed *before* the details).
  std::ostringstream Buffer;
  bool InTestSuiteFile = false;
  std::string CurrentFile;
  int CurrentFilePassed = 0;
  int CurrentFileFailed = 0;
  int CurrentFileSkipped = 0;

  // Aggregate stats.
  int FilesPassed = 0;
  int FilesFailed = 0;
  std::chrono::steady_clock::time_point StartTime;
  std::vector<FailureRecord> Failures;
  std::vector<HookErrorRecord> HookErrors;

  // Tracks how deep into named describe() blocks we currently are *within*
  // a TEST_SUITE wrapper. Incremented in onSuiteStart, decremented in
  // onSuiteEnd. Reset whenever we enter a new IsTestSuiteRoot wrapper.
  int DescribeDepth = 0;
  int NamedDescribeOpen = 0;

  std::ostream &out() { return InTestSuiteFile ? Buffer : std::cout; }

  static void indent(std::ostream &os, int n) {
    for (int i = 0; i < n; ++i)
      os << "  ";
  }

  static void appendDuration(std::ostream &os,
                             std::chrono::nanoseconds duration) {
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    // Jest only shows the timing if it's >= 1 ms, which keeps fast tests
    // visually clean.
    if (ms >= 1) {
      os << " " << utils::gray() << "(" << ms << " ms)" << utils::reset();
    }
  }

  void printSummaryJestStyle(int passed, int failed, int skipped,
                             long long elapsedMs) {
    // Test Suites line.
    std::cout << utils::bold() << "Test Suites: " << utils::reset();
    if (FilesFailed)
      std::cout << utils::red() << utils::bold() << FilesFailed << " failed"
                << utils::reset() << ", ";
    if (FilesPassed)
      std::cout << utils::green() << utils::bold() << FilesPassed << " passed"
                << utils::reset() << ", ";
    std::cout << (FilesPassed + FilesFailed) << " total\n";

    // Tests line.
    std::cout << utils::bold() << "Tests:       " << utils::reset();
    if (failed)
      std::cout << utils::red() << utils::bold() << failed << " failed"
                << utils::reset() << ", ";
    if (skipped)
      std::cout << utils::yellow() << utils::bold() << skipped << " skipped"
                << utils::reset() << ", ";
    if (passed)
      std::cout << utils::green() << utils::bold() << passed << " passed"
                << utils::reset() << ", ";
    std::cout << (passed + failed + skipped) << " total\n";

    // Snapshots (always 0 for now — Cest doesn't have snapshots).
    std::cout << utils::bold() << "Snapshots:   " << utils::reset()
              << "0 total\n";

    // Time. Jest also appends ", estimated <ceil> s" — we mimic that.
    double seconds = static_cast<double>(elapsedMs) / 1000.0;
    long long estimated = static_cast<long long>(seconds + 0.999);
    if (estimated < 1)
      estimated = 1;
    std::cout << utils::bold() << "Time:        " << utils::reset()
              << std::fixed << std::setprecision(3) << seconds << " s"
              << ", estimated " << estimated << " s\n";
  }

  void printSummaryTable(int passed, int failed, int skipped,
                         long long elapsedMs) {
#ifdef CEST_USE_TABULATE
    tabulate::Table t;
    t.add_row({"Metric", "Value"});

    {
      std::ostringstream os;
      if (FilesFailed)
        os << FilesFailed << " failed, ";
      if (FilesPassed)
        os << FilesPassed << " passed, ";
      os << (FilesPassed + FilesFailed) << " total";
      t.add_row({"Test Suites", os.str()});
    }
    {
      std::ostringstream os;
      if (failed)
        os << failed << " failed, ";
      if (skipped)
        os << skipped << " skipped, ";
      if (passed)
        os << passed << " passed, ";
      os << (passed + failed + skipped) << " total";
      t.add_row({"Tests", os.str()});
    }
    t.add_row({"Snapshots", "0 total"});
    {
      std::ostringstream os;
      os << std::fixed << std::setprecision(3)
         << (static_cast<double>(elapsedMs) / 1000.0) << " s";
      t.add_row({"Time", os.str()});
    }

    t.format().border_color(tabulate::Color::grey);
    t[0].format()
        .font_style({tabulate::FontStyle::bold})
        .font_color(tabulate::Color::white);

    std::cout << t << "\n";
#else
    // Fallback to Jest-style if tabulate isn't compiled in.
    printSummaryJestStyle(passed, failed, skipped, elapsedMs);
#endif
  }
};

} // namespace reporters

} // namespace cest

#endif // CONSOLE_REPORTER_HPP