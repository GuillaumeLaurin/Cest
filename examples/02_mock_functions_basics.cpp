#include "cest.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace cest;

template <typename Sender> class Notifier {
public:
  explicit Notifier(Sender sender) : Value(std::move(sender)) {}

  bool notify(const std::string &user, const std::string &msg) {
    if (user.empty())
      return false;
    Value(user, msg);
    return true;
  }

private:
  Sender Value;
};

TEST_SUITE(MockFunctionBasics) {
  describe("MockFn<void(string, string)>", [] {
    it("records calls and reports count", [] {
      MockFn<void(std::string, std::string)> sender;
      Notifier<MockFn<void(std::string, std::string)>> n(sender);

      n.notify("alice", "hi");
      n.notify("bob", "yo");

      expect(sender).toHaveBeenCalled();
      expect(sender).toHaveBeenCalledTimes(2);
      expect(sender).toHaveBeenCalledWith(std::string("alice"),
                                          std::string("hi"));
      expect(sender).toHaveBeenCalledWith(std::string("bob"),
                                          std::string("yo"));
    });
    it("empty-user path does not call the sender", [] {
      MockFn<void(std::string, std::string)> sender;
      Notifier<MockFn<void(std::string, std::string)>> n(sender);

      bool r = n.notify("", "hi");
      expect(r).toBe(false);
      expect(sender).Not().toHaveBeenCalled();
    });
  });

  describe("MockFn<int(int, int)> with beforeEach", [] {
    static std::shared_ptr<MockFn<int(int, int)>> add;

    beforeEach([] { add = std::make_shared<MockFn<int(int, int)>>(); });

    it("mockReturnValue returns the programmed value", [] {
      add->mockReturnValue(42);
      expect((*add)(1, 2)).toBe(42);
      expect((*add)(5, 5)).toBe(42);
      expect(*add).toHaveBeenCalledTimes(2);
    });

    it("mockReturnValueOnce queues one-shot value", [] {
      add->mockReturnValue(0);
      add->mockReturnValueOnce(10).mockReturnValueOnce(20);
      expect((*add)(1, 1)).toBe(10);
      expect((*add)(1, 1)).toBe(20);
      expect((*add)(1, 1)).toBe(0);
    });

    it("mockImplementation runs arbitrary logic", [] {
      add->mockImplementation([](int a, int b) { return a + b; });
      expect((*add)(2, 3)).toBe(5);
    });

    it("mockClear wipes history but keeps configuration", [] {
      add->mockReturnValue(7);
      (*add)(1, 1);
      (*add)(2, 2);
      expect(*add).toHaveBeenCalledTimes(2);
      add->mockClear();
      expect(*add).toHaveBeenCalledTimes(0);
      expect((*add)(0, 0)).toBe(7);
    });
  });
}