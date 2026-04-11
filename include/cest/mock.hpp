#ifndef MOCK_HPP
#define MOCK_HPP
// -*-- C++ -*- Header-only

/**
 * @file include/mock.hpp
 *
 * Philosophy :
 *  * No virtual base classes, no interface pollution.
 *  * Parameterize code-under-test by a *dependency type* (template) or by
 *    std::function. In tests, inject a MockFn<R(Args...)>.
 *  * MockFn records every call, supports mockReturnValue / mockImplementation /
 *    mockReturnValueOnce, and exposes Jest-style matchers via expect().
 */
#include "core.hpp"

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace cest {

template <typename Signature> class MockFn;

template <typename R, typename... Args> class MockFn<R(Args...)> {
public:
  using ArgsTuple = std::tuple<std::decay_t<Args>...>;
  using ImplType = std::function<R(Args...)>;

  MockFn() : Value(std::make_shared<State>()) {}

  MockFn &mockImplementation(ImplType impl) {
    std::lock_guard<std::mutex> g(Value->Mutex);
    Value->Impl = std::move(impl);
    return *this;
  }

  template <typename U = R, std::enable_if_t<!std::is_void_v<U>, int> = 0>
  MockFn &mockReturnValue(U value) {
    std::lock_guard<std::mutex> g(Value->Mutex);
    Value->DefaultReturn = std::move(value);
    return *this;
  }

  template <typename U = R, std::enable_if_t<!std::is_void_v<U>, int> = 0>
  MockFn &mockReturnValueOnce(U value) {
    std::lock_guard<std::mutex> g(Value->Mutex);
    Value->OnceQueue.push_back(std::move(value));
    return *this;
  }

  MockFn &mockReset() {
    std::lock_guard<std::mutex> g(Value->Mutex);
    Value->Calls.clear();
    Value->OnceQueue.clear();
    Value->DefaultReturn.clear();
    Value->Impl = nullptr;
    return *this;
  }

  MockFn &mockClear() {
    std::lock_guard<std::mutex> g(Value->Mutex);
    Value->Calls.clear();
    return *this;
  }

  R operator()(Args... args) {
    ArgsTuple captured(std::forward<Args>(args)...);
    {
      std::lock_guard<std::mutex> g(Value->Mutex);
      Value->Calls.push_back(captured);
    }
    if constexpr (std::is_void_v<R>) {
      if (Value->Impl) {
        std::apply(Value->Impl, std::move(captured));
      }
      return;
    } else {
      {
        std::lock_guard<std::mutex> g(Value->Mutex);
        if (!Value->OnceQueue.empty()) {
          R v = std::move(Value->OnceQueue.front());
          Value->OnceQueue.pop_front();
          return v;
        }
      }
      if (Value->Impl) {
        return std::apply(Value->Impl, std::move(captured));
      }
      {
        std::lock_guard<std::mutex> g(Value->Mutex);
        if (Value->DefaultReturn.has_value()) {
          return *Value->DefaultReturn;
        }
      }
      if constexpr (std::is_default_constructible_v<R>) {
        return R{};
      } else {
        throw std::runtime_error(
            "MockFn: no return configured for non-default-constructible R");
      }
    }
  }

  std::vector<ArgsTuple> calls() const {
    std::lock_guard<std::mutex> g(Value->Mutex);
    return Value->Calls;
  }

  size_t callCount() const {
    std::lock_guard<std::mutex> g(Value->Mutex);
    return Value->Calls.size();
  }

  bool wasCalled() const { return callCount() > 0; }
  bool wasCalledTimes(size_t n) const { return callCount() == n; }

  bool wasCalledWith(const std::decay_t<Args> &...expected) const {
    auto want = std::make_tuple(expected...);
    std::lock_guard<std::mutex> g(Value->Mutex);
    for (const auto &c : Value->Calls) {
      if (c == want)
        return true;
    }
    return false;
  }

  std::string describeCalls() const {
    std::lock_guard<std::mutex> g(Value->Mutex);
    std::ostringstream os;
    os << "[" << Value->Calls.size() << " call(s)]";
    return os.str();
  }

private:
  struct State {
    mutable std::mutex Mutex;
    std::vector<ArgsTuple> Calls;
    std::optional<R> DefaultReturn;
    std::deque<R> OnceQueue;
    ImplType Impl;
  };

  std::shared_ptr<State> Value;
};

template <typename... Args> class MockFn<void(Args...)> {
public:
  using ArgsTuple = std::tuple<std::decay_t<Args>...>;
  using ImplType = std::function<void(Args...)>;

  MockFn() : Value(std::make_shared<State>()) {}

  MockFn &mockImplementation(ImplType impl) {
    std::lock_guard<std::mutext> g(Value->Mutex);
    Value->Impl = std::move(impl);
    return *this;
  }

  MockFn &mockReset() {
    std::lock_guard<std::mutex> g(Value->Mutex);
    Value->Calls.clear();
    Value->OnceQueue.clear();
    Value->DefaultReturn.clear();
    Value->Impl = nullptr;
    return *this;
  }

  MockFn &mockClear() {
    std::lock_guard<std::mutex> g(Value->Mutex);
    Value->Calls.clear();
    return *this;
  }

  void operator()(Args... args) {
    ArgsTuple captured(std::forward<Args>(args)...);
    {
      std::lock_guard<std::mutex> g(Value->Mutex);
      Value->Calls.push_back(captured);
    }
    if (Value->Impl) {
      std::apply(Value->Impl, std::move(captured));
    }
  }

  std::vector<ArgsTuple> calls() const {
    std::lock_guard<std::mutex> g(Value->Mutex);
    return Value->Calls;
  }

  size_t callCount() const {
    std::lock_guard<std::mutex> g(Value->Mutex);
    return Value->Calls.size();
  }

  bool wasCalled() const { return callCount() > 0; }
  bool wasCalledTimes(size_t n) const { return callCount() == n; }

  bool wasCalledWith(const std::decay_t<Args> &...expected) const {
    auto want = std::make_tuple(expected...);
    std::lock_guard<std::mutex> g(Value->Mutex);
    for (const auto &c : Value->Calls) {
      if (c == want)
        return true;
    }
    return false;
  }

  std::string describeCalls() const {
    std::lock_guard<std::mutex> g(Value->Mutex);
    std::ostringstream os;
    os << "[" << Value->Calls.size() << " call(s)]";
    return os.str();
  }

private:
  struct State {
    mutable std::mutex Mutex;
    std::vector<ArgsTuple> Calls;
    ImplType Impl;
  };

  std::shared_ptr<State> Value;
};

template <typename Sig> class MockExpectation {
public:
  MockExpectation(MockFn<Sig> mock, bool negated = false)
      : Mock(std::move(mock)), Negated(negated) {}

  MockExpectation<Sig> Not() const {
    return MockExpectation<Sig>(Mock, !Negated);
  }

  void toHaveBeenCalled() const {
    bool r = Mock.wasCalled();
    if (r == Negated)
      fail("toHaveBeenCalled", "");
  }

  void toHaveBeenCalledTimes(size_t n) const {
    bool r = Mock.wasCalledTimes(n);
    if (r == Negated) {
      std::ostringstream os;
      os << n << " (actual: " << Mock.callCount() << ")";
      fail("toHaveBeenCalledTimes", os.str());
    }
  }

  template <typename... Expected>
  void toHaveBeenCalledWith(const Expected &...expected) const {
    bool r = Mock.wasCalledWith(expected...);
    if (r == Negated)
      fail("toHaveBeenCalledWith", Mock.describeCalls());
  }

private:
  MockFn<Sig> Mock;
  bool Negated;

  void fail(const char *matcher, const std::string &extra) const {
    std::ostringstream os;
    os << "expect(mock)" << (Negated ? ".not." : ".") << matcher << "(" << extra
       << ")";
    throw AssertionError(os.str());
  }
};

template <typename Sig> MockExpectation<Sig> expect(MockFn<Sig> mock) {
  return MockExpectation<Sig>(std::move(mock));
}

} // namespace cest

#endif // MOCK_HPP