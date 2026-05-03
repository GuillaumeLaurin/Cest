#ifndef I_REPORTER_HPP
#define I_REPORTER_HPP

#include "cest/utils/suite.hpp"

#include "cest/utils/test_case.hpp"

#include <chrono>
#include <string>

namespace cest {

namespace reporters {

/**
 * @brief Interface implemented by every reporter.
 *
 * The Runner drives the reporter through a strict sequence of events:
 *
 *   onRunStart
 *     [ onFocussed ]                    (only if any test/suite is focussed)
 *     onSuiteStart  (per suite, depth-first, includes the synthetic
 *                    TEST_SUITE wrapper with IsTestSuiteRoot == true)
 *       onTestPass | onTestFail | onTestSkip   (per test in the suite)
 *       onHookError                            (when a hook throws)
 *     onSuiteEnd
 *   onRunEnd
 */
class IReporter {
public:
  virtual ~IReporter() = default;

  virtual void onFocussed() = 0;

  virtual void onRunStart(uint64_t seed, bool passed) = 0;

  virtual void onSuiteStart(const utils::Suite &suite, int depth) = 0;

  virtual void onSuiteEnd(const utils::Suite &suite, int depth) = 0;

  virtual void onTestPass(const utils::TestCase &test,
                          std::chrono::nanoseconds duration) = 0;

  virtual void onTestFail(const utils::TestCase &test, const std::string &err,
                          std::chrono::nanoseconds duration) = 0;

  virtual void onTestSkip(const utils::TestCase &test) = 0;

  /**
   * @brief Called whenever a beforeAll/afterAll/beforeEach/afterEach hook
   *        throws.
   * @param suite     The suite the hook belongs to.
   * @param hookName  One of "beforeAll", "afterAll", "beforeEach",
   *                  "afterEach".
   * @param message   The exception's what(), or "unknown throw".
   */
  virtual void onHookError(const utils::Suite &suite,
                           const std::string &hookName,
                           const std::string &message) = 0;

  virtual void onRunEnd(int passed, int failed, int skipped) = 0;
};

} // namespace reporters

} // namespace cest

#endif // I_REPORTER_HPP