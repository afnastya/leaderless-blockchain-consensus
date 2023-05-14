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

  std::optional<Message> receive() {
    return std::nullopt;
  }

  void handle_messages() override {
  }

  void stop_receive() override {
  }

  void broadcast(Message) override {
  }

  void close() override {
  }

  void set_timer(uint32_t, std::function<void()>) override {
  }

  void add_message(const Message& msg) {
    queue.push_back(std::move(msg));
  }

private:
  std::deque<Message> queue;
};

std::deque<Message> get_RB_msgs(int n, int t) {
  static auto get_data = [](int value) { return json{{"value", value}}; };
  std::deque<Message> RB_msgs;

  for (int i = 0; i + t < n; ++i) {
    RB_msgs.push_back(Message("RB_ECHO", get_data(0), i));
  }

  for (int i = 0; i + t < n; ++i) {
    RB_msgs.push_back(Message("RB_READY", get_data(0), i));
  }

  return RB_msgs;
}

void sanity_check(int n, int t) {
  static auto get_data = [](int value) { return json{{"value", value}}; };

  FakeNetManager net_manager;
  ReliableBroadcast RB(n, net_manager);

  RB.broadcast(get_data(0));

  auto RB_msgs = get_RB_msgs(n, t);

  for (int i = 0; i + t < n; ++i) {
    auto msg = std::move(RB_msgs.front());
    RB_msgs.pop_front();
    auto res = RB.process_msg(msg);
    EXPECT_EQ(res, std::nullopt);
  }

  for (int i = 0; i + t + 1 < n; ++i) {
    auto msg = std::move(RB_msgs.front());
    RB_msgs.pop_front();
    auto res = RB.process_msg(msg);
    EXPECT_EQ(res, std::nullopt);
  }

  auto msg = std::move(RB_msgs.front());
  RB_msgs.pop_front();
  auto res = RB.process_msg(msg);
  ASSERT_NE(res, std::nullopt);
  EXPECT_EQ(res.value(), get_data(0));
  EXPECT_EQ(RB.is_delivered(get_data(0)), true);
}


TEST(ReliableBroadcast, JustWorks) {
  for (size_t n = 4; n < 50; n += 3) {
    sanity_check(n, (n - 1) / 3);
  }
}

void shuffle_check(int n, int t) {
  static auto get_data = [](int value) { return json{{"value", value}}; };

  FakeNetManager net_manager;
  ReliableBroadcast RB(n, net_manager);

  RB.broadcast(get_data(0));

  auto RB_msgs = get_RB_msgs(n, t);
  std::mt19937 engine(std::random_device{}());
  std::shuffle(std::begin(RB_msgs), std::end(RB_msgs), engine);

  while (!RB_msgs.empty()) {
    auto msg = std::move(RB_msgs.front());
    RB_msgs.pop_front();
    auto res = RB.process_msg(msg);
  }

  EXPECT_EQ(RB.is_delivered(get_data(0)), true);
}

TEST(ReliableBroadcast, Shuffle) {
  for (size_t n = 4; n < 50; n += 3) {
    shuffle_check(n, (n - 1) / 3);
  }
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