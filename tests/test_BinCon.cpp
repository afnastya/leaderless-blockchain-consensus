#include <gtest/gtest.h>
#include <glog/logging.h>
#include <iostream>

#include <deque>
#include <optional>
#include <functional>
#include <cstdlib>

#include <../src/core/message.hpp>
#include <../src/network/netmanager.hpp>
#include <../src/network/network.hpp>
#include <../src/consensus/ReliableBroadcast.hpp>
#include <../src/consensus/BinConsensus.hpp>
// #include "../src/logger.cpp"

using namespace std::chrono_literals;

class BinaryNode {
public:
    uint32_t id_;
    INetManager* net_manager_;
    std::thread msg_processor_;
    bool decision = false;

    BinaryNode(uint32_t id, INetwork& net) : id_(id) {
        net_manager_ = new NetManager(id, net.add_node(id), net);
    }
    // Node(Node&& other);
    void run(uint32_t proposal = 1) {
        net_manager_->update_nodes();
        std::thread thread([this, proposal]() {
            BinConsensus bin_con(net_manager_->get_nodes_cnt(), *net_manager_);
            bin_con.bin_propose(proposal);

            std::optional<Message> msg_opt;
            while ((msg_opt = net_manager_->receive()) != std::nullopt) {
                auto msg = msg_opt.value();

                if (bin_con.process_msg(msg)) {
                    decision = bin_con.get_decision();
                    break;
                }
            }
        });

        msg_processor_ = std::move(thread);
    }

    void join() {
        msg_processor_.join();
    }

    uint32_t get_id() const {
        return id_;
    }
};

void sanity_check(int n, std::function<uint32_t(uint32_t)> gen_proposal) {
    ManualNetwork net;
    std::vector<BinaryNode> nodes;

    for (int i = 0; i < n; ++i) {
        nodes.emplace_back(i, net);
    }
    for (int i = 0; i < n; ++i) {
        nodes[i].run(gen_proposal(i));
    }

    net.run_detached();

    for (int i = 0; i < n; ++i) {
        nodes[i].join();
    }

    net.shutdown();

    for (int i = 1; i < n; ++i) {
        EXPECT_EQ(nodes[0].decision, nodes[i].decision);
    }
}

TEST(BinConsensus, JustWorks) {
    auto gen_one = [](uint32_t) { return 1;};
    auto gen_mod = [](uint32_t id) { return id % 2;};
    auto gen_rand = [](uint32_t id) { return std::rand() % 2;};

    for (int n = 4; n < 20; ++n) {
        sanity_check(n, gen_one);
        sanity_check(n, gen_mod);
        sanity_check(n, gen_rand);
    }
}

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
    return std::nullopt;
  }

  void broadcast(Message) override {
  }

void close() override {
  }
};

void queue_test(int n, int t) {
    FakeNetManager net_manager;
    BinConsensus BinCon(n, net_manager);
}

void shuffle_check(int n, std::function<uint32_t(uint32_t)> gen_proposal) {
    ManualNetwork net;
    std::vector<BinaryNode> nodes;

    for (int i = 0; i < n; ++i) {
        nodes.emplace_back(i, net);
    }
    for (int i = 0; i < n; ++i) {
        nodes[i].run(gen_proposal(i));
    }

    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(20ms);
        net.shuffle();
        net.shuffle();
        net.shuffle();
        net.step_n(n * n * 5);
    }

    LOG(INFO) << "TEST BinConsensus.MixedOrder: all shuffles are done" << std::endl;
    net.run_detached();

    for (int i = 0; i < n; ++i) {
        nodes[i].join();
    }

    net.shutdown();

    for (int i = 1; i < n; ++i) {
        EXPECT_EQ(nodes[0].decision, nodes[i].decision);
    }
}

TEST(BinConsensus, MixedOrder) {
    auto gen_one = [](uint32_t) { return 1;};
    auto gen_mod = [](uint32_t id) { return id % 2;};
    auto gen_rand = [](uint32_t id) { return std::rand() % 2;};

    for (int n = 10; n < 17; ++n) {
        shuffle_check(n, gen_one);
        shuffle_check(n, gen_mod);
        shuffle_check(n, gen_rand);
    }
}

int main(int argc, char **argv) {
    FLAGS_log_dir = "./log";
    google::InitGoogleLogging(argv[0]);
    // google::SetCommandLineOption("GLOG_minloglevel", "4");

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}