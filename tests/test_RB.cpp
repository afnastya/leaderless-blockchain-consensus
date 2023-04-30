#include <gtest/gtest.h>

#include <deque>
#include <optional>

#include <../src/core/message.hpp>
#include <../src/network/netmanager.hpp>
#include <../src/consensus/ReliableBroadcast.hpp>

class FakeNetManager : public INetManager {
public:
  void update_nodes() override {
  }

  uint32_t get_nodes_cnt() const override {
      return 0;
  }

  uint32_t get_id() const override {
    return 0;
  }

  void send(Message) override {
  }

  std::optional<Message> receive() override {
    if (queue.empty()) {
      return std::nullopt;
    }

    Message msg = std::move(queue.front());
    queue.pop_front();

    return msg;
  }

  void broadcast(Message) override {
  }

  void close() override {
  }

  void add_message(const Message& msg) {
    queue.push_back(std::move(msg));
  }

private:
  std::deque<Message> queue;
};

void sanity_check(int n, int t) {
  auto get_data = [](int value) { return json{{"value", value}}; };

  FakeNetManager net_manager;
  ReliableBroadcast RB(n, net_manager);

  RB.broadcast(get_data(0));

  for (int i = 0; i + t < n; ++i) {
    auto res = RB.process_msg(Message("RB_ECHO", get_data(0), i));
    EXPECT_EQ(res, std::nullopt);
  }

  for (int i = 0; i + t + 1 < n; ++i) {
    auto res = RB.process_msg(Message("RB_READY", get_data(0), i));
    EXPECT_EQ(res, std::nullopt);
  }

  auto res = RB.process_msg(Message("RB_READY", get_data(0), n - 1));
  ASSERT_NE(res, std::nullopt);
  EXPECT_EQ(res.value(), get_data(0));
}


TEST(ReliableBroadcast, JustWorks) {
  sanity_check(4, 1);
  sanity_check(5, 1);
  sanity_check(6, 1);
  sanity_check(10, 3);
  sanity_check(49, 16);
}


TEST(ReliableBroadcast, MultipleBroadcasts) {
  auto get_data = [](int value) { return json{{"value", value}}; };
  int n = 10, t = 3;

  FakeNetManager net_manager;
  ReliableBroadcast RB(n, net_manager);

  for (int value = 0; value < 5; ++value) {
    RB.broadcast(get_data(value));
  }

  for (int i = 0; i + t < n; ++i) {
    for (int value = 0; value < 3; ++value) {
      auto res = RB.process_msg(Message("RB_ECHO", get_data(value), i));
      EXPECT_EQ(res, std::nullopt);
    }
  }

  for (int i = 0; i + t + 1 < n; ++i) {
    for (int value = 0; value < 5; ++value) {
      auto res = RB.process_msg(Message("RB_READY", get_data(value), i));
      EXPECT_EQ(res, std::nullopt);
    }
  }

  for (int value = 0; value < 5; ++value) {
    auto res = RB.process_msg(Message("RB_READY", get_data(value), n - 1));
    ASSERT_NE(res, std::nullopt);
    EXPECT_EQ(res.value(), get_data(value));
  }
}


TEST(ReliableBroadcast, GetBroadcast) {
  auto get_data = [](int value) { return json{{"value", value}}; };
  int n = 10, t = 3;

  FakeNetManager net_manager;
  ReliableBroadcast RB(n, net_manager);

  for (int i = 0; i + t < n; ++i) {
    auto res = RB.process_msg(Message("RB_ECHO", get_data(0), i));
    EXPECT_EQ(res, std::nullopt);
    res = RB.process_msg(Message("RB_ECHO", get_data(1), i));
    EXPECT_EQ(res, std::nullopt);
  }

  RB.broadcast(get_data(5));

  for (int i = 0; i + t + 1 < n; ++i) {
    auto res = RB.process_msg(Message("RB_READY", get_data(5), i));
    EXPECT_EQ(res, std::nullopt);

    res = RB.process_msg(Message("RB_READY", get_data(0), i));
    EXPECT_EQ(res, std::nullopt);
  }

  auto res = RB.process_msg(Message("RB_READY", get_data(5), n - 1));
  ASSERT_NE(res, std::nullopt);
  EXPECT_EQ(res.value(), get_data(5));

  res = RB.process_msg(Message("RB_READY", get_data(0), n - 1));
  ASSERT_NE(res, std::nullopt);
  EXPECT_EQ(res.value(), get_data(0));
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}