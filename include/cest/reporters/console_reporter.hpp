#ifndef CONSOLE_REPORTER_HPP
#define CONSOLE_REPORTER_HPP

#include "cest/reporters/i_reporter.hpp"

#include "cest/utils/colors.hpp"

#include "cest/utils/suite.hpp"

#include "cest/utils/test_case.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifdef CEST_USE_TABULATE
#include <tabulate/table.hpp>
#endif

namespace cest {

namespace reporters {

class ConsoleReporter : public IReporter {
public:
  /**
   * @param useTable    If true, the final summary block is rendered as a
   *                    tabulate table with borders. Otherwise it uses Jest's
   *                    plain column-aligned format.
   * @param autoCompact If true (default), when more than one test file ran
   *                    we omit passing tests from the per-file detail. Set
   *                    to false to always show every test.
   */
  explicit ConsoleReporter(bool useTable = false, bool autoCompact = true)
      : UseTable(useTable), AutoCompact(autoCompact) {}

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
    Files.clear();
    Failures.clear();
    HookErrors.clear();
  }

  void onSuiteStart(const utils::Suite &suite, int /*depth*/) override {
    if (suite.IsTestSuiteRoot) {
      Files.emplace_back();
      Files.back().Path = suite.File;
      NodeStack.clear();
      NodeStack.push_back(&Files.back().Tree);
      InTestSuiteFile = true;
      return;
    }

    if (!InTestSuiteFile)
      return;

    auto child = std::make_unique<Node>();
    child->Kind = NodeKind::Describe;
    child->Name = suite.Name;
    child->Skipped = suite.Skipped;
    Node *raw = child.get();
    NodeStack.back()->Children.push_back(std::move(child));
    NodeStack.push_back(raw);
  }

  void onSuiteEnd(const utils::Suite &suite, int /*depth*/) override {
    if (suite.IsTestSuiteRoot) {
      InTestSuiteFile = false;
      NodeStack.clear();
      bool fileFailed =
          Files.back().FailedCount > 0 || Files.back().HookErrorCount > 0;
      if (fileFailed)
        ++FilesFailed;
      else
        ++FilesPassed;
      return;
    }

    if (!InTestSuiteFile)
      return;

    if (NodeStack.size() > 1)
      NodeStack.pop_back();
  }

  void onTestPass(const utils::TestCase &test,
                  std::chrono::nanoseconds duration) override {
    if (!InTestSuiteFile)
      return;
    auto leaf = std::make_unique<Node>();
    leaf->Kind = NodeKind::TestPass;
    leaf->Name = test.Name;
    leaf->Duration = duration;
    NodeStack.back()->Children.push_back(std::move(leaf));
    ++Files.back().PassedCount;
  }

  void onTestFail(const utils::TestCase &test, const std::string &err,
                  std::chrono::nanoseconds duration) override {
    if (!InTestSuiteFile)
      return;
    auto leaf = std::make_unique<Node>();
    leaf->Kind = NodeKind::TestFail;
    leaf->Name = test.Name;
    leaf->Message = err;
    leaf->Duration = duration;
    NodeStack.back()->Children.push_back(std::move(leaf));
    ++Files.back().FailedCount;

    Failures.push_back({Files.back().Path, test.Name, err});
  }

  void onTestSkip(const utils::TestCase &test) override {
    if (!InTestSuiteFile)
      return;
    auto leaf = std::make_unique<Node>();
    leaf->Kind = NodeKind::TestSkip;
    leaf->Name = test.Name;
    NodeStack.back()->Children.push_back(std::move(leaf));
    ++Files.back().SkippedCount;
  }

  void onHookError(const utils::Suite &suite, const std::string &hookName,
                   const std::string &message) override {
    if (!InTestSuiteFile)
      return;
    auto leaf = std::make_unique<Node>();
    leaf->Kind = NodeKind::HookError;
    leaf->Name = hookName;
    leaf->Message = message;
    leaf->SuiteName = suite.Name;
    NodeStack.back()->Children.push_back(std::move(leaf));
    ++Files.back().HookErrorCount;

    HookErrors.push_back({Files.back().Path, suite.Name, hookName, message});
  }

  void onRunEnd(int passed, int failed, int skipped) override {
    auto end = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - StartTime)
            .count();

    bool compact = AutoCompact && Files.size() > 1;

    for (const auto &file : Files) {
      printBanner(file);

      if (compact && file.FailedCount == 0 && file.SkippedCount == 0 &&
          file.HookErrorCount == 0) {
        continue;
      }
      for (const auto &child : file.Tree.Children) {
        renderNode(*child, /*depth=*/0, compact);
      }
    }

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
  enum class NodeKind { Describe, TestPass, TestFail, TestSkip, HookError };

  struct Node {
    NodeKind Kind = NodeKind::Describe;
    std::string Name;
    std::string Message;
    std::string SuiteName;
    std::chrono::nanoseconds Duration{0};
    bool Skipped = false;
    std::vector<std::unique_ptr<Node>> Children;
  };

  struct FileRecord {
    std::string Path;
    Node Tree;
    int PassedCount = 0;
    int FailedCount = 0;
    int SkippedCount = 0;
    int HookErrorCount = 0;
  };

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
  bool AutoCompact = true;

  std::vector<FileRecord> Files;
  std::vector<Node *> NodeStack;
  bool InTestSuiteFile = false;

  int FilesPassed = 0;
  int FilesFailed = 0;
  std::chrono::steady_clock::time_point StartTime;

  std::vector<FailureRecord> Failures;
  std::vector<HookErrorRecord> HookErrors;

  static void indent(std::ostream &os, int n) {
    for (int i = 0; i < n; ++i)
      os << "  ";
  }

  static bool subtreeHasNonPass(const Node &n) {
    if (n.Kind == NodeKind::TestFail || n.Kind == NodeKind::TestSkip ||
        n.Kind == NodeKind::HookError)
      return true;
    for (const auto &c : n.Children) {
      if (subtreeHasNonPass(*c))
        return true;
    }
    return false;
  }

  void renderNode(const Node &n, int depth, bool compact) const {
    switch (n.Kind) {
    case NodeKind::Describe: {

      if (compact && !subtreeHasNonPass(n))
        return;

      if (!n.Name.empty()) {
        indent(std::cout, depth + 1);
        if (n.Skipped) {
          std::cout << utils::bold() << utils::yellow() << n.Name
                    << utils::reset() << "\n";
        } else {
          std::cout << utils::bold() << n.Name << utils::reset() << "\n";
        }
      }
      int childDepth = n.Name.empty() ? depth : depth + 1;
      for (const auto &c : n.Children)
        renderNode(*c, childDepth, compact);
      break;
    }
    case NodeKind::TestPass: {
      if (compact)
        return;
      indent(std::cout, depth + 1);
      std::cout << utils::green() << "\xE2\x9C\x93 " << utils::reset()
                << n.Name;
      appendDuration(std::cout, n.Duration);
      std::cout << "\n";
      break;
    }
    case NodeKind::TestFail: {
      indent(std::cout, depth + 1);
      std::cout << utils::red() << "\xE2\x9C\x97 " << utils::reset() << n.Name;
      appendDuration(std::cout, n.Duration);
      std::cout << "\n";
      indent(std::cout, depth + 2);
      std::cout << utils::red() << n.Message << utils::reset() << "\n";
      break;
    }
    case NodeKind::TestSkip: {
      indent(std::cout, depth + 1);
      std::cout << utils::yellow() << "\xE2\x97\x8B " << utils::reset()
                << utils::gray() << "skipped " << n.Name << utils::reset()
                << "\n";
      break;
    }
    case NodeKind::HookError: {
      indent(std::cout, depth + 1);
      std::cout << utils::red() << n.Name << " threw"
                << (n.SuiteName.empty() ? "" : " in \"" + n.SuiteName + "\"")
                << ": " << n.Message << utils::reset() << "\n";
      break;
    }
    }
  }

  void printBanner(const FileRecord &file) const {
    bool failed = file.FailedCount > 0 || file.HookErrorCount > 0;
    if (failed) {
      std::cout << utils::bold() << utils::black() << utils::bgRed() << " FAIL "
                << utils::reset() << " " << utils::bold() << file.Path
                << utils::reset() << "\n";
    } else {
      std::cout << utils::bold() << utils::black() << utils::bgGreen()
                << " PASS " << utils::reset() << " " << utils::bold()
                << file.Path << utils::reset() << "\n";
    }
  }

  static void appendDuration(std::ostream &os,
                             std::chrono::nanoseconds duration) {
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    if (ms >= 1) {
      os << " " << utils::gray() << "(" << ms << " ms)" << utils::reset();
    }
  }

  void printSummaryJestStyle(int passed, int failed, int skipped,
                             long long elapsedMs) {
    std::cout << utils::bold() << "Test Suites: " << utils::reset();
    if (FilesFailed)
      std::cout << utils::red() << utils::bold() << FilesFailed << " failed"
                << utils::reset() << ", ";
    if (FilesPassed)
      std::cout << utils::green() << utils::bold() << FilesPassed << " passed"
                << utils::reset() << ", ";
    std::cout << (FilesPassed + FilesFailed) << " total\n";

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

    std::cout << utils::bold() << "Snapshots:   " << utils::reset()
              << "0 total\n";

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
    printSummaryJestStyle(passed, failed, skipped, elapsedMs);
#endif
  }
};

} // namespace reporters

} // namespace cest

#endif // CONSOLE_REPORTER_HPP