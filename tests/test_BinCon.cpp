#include <gtest/gtest.h>
#include <glog/logging.h>
#include <iostream>

#include <deque>
#include <optional>
#include <functional>
#include <cstdlib>
#include <memory>

#include <../src/core/message.hpp>
#include <../src/network/netmanager.hpp>
#include <../src/network/network.hpp>
#include <../src/consensus/ReliableBroadcast.hpp>
#include <../src/consensus/BinConsensus.hpp>

using namespace std::chrono_literals;

class BinaryNode {
public:
    uint32_t id_;
    INetManager* net_manager_;
    std::thread msg_processor_;
    BinConsensus* bin_con;
    bool decision = false;

    BinaryNode(uint32_t id, INetwork& net) : id_(id) {
        auto handler = [this](Message msg) {
            this->handle_message(msg);
        };

        net_manager_ = new NetManager(id, net.add_node(id), net, handler);
    }

    void run(uint32_t proposal = 1) {
        net_manager_->update_nodes();
        std::thread thread([this, proposal]() {
            bin_con = new BinConsensus(net_manager_->get_nodes_cnt(), *net_manager_);
            bin_con->bin_propose(proposal);

            net_manager_->handle_messages();
        });

        msg_processor_ = std::move(thread);
    }

    void handle_message(Message msg) {
        if (bin_con->process_msg(msg)) {
            decision = bin_con->get_decision();
            net_manager_->stop_receive();
        }
    }

    void join() {
        msg_processor_.join();
    }

    uint32_t get_id() const {
        return id_;
    }

    ~BinaryNode() {
        delete bin_con;
        delete net_manager_;
    }
};

void sanity_check(int n, std::function<uint32_t(uint32_t)> gen_proposal) {
    ManualNetwork net;
    std::vector<std::unique_ptr<BinaryNode>> nodes;

    for (int i = 0; i < n; ++i) {
        nodes.emplace_back(std::make_unique<BinaryNode>(i, net));
    }
    for (int i = 0; i < n; ++i) {
        nodes[i]->run(gen_proposal(i));
    }

    net.run_detached();

    for (int i = 0; i < n; ++i) {
        nodes[i]->join();
    }

    net.shutdown();

    for (int i = 1; i < n; ++i) {
        EXPECT_EQ(nodes[0]->decision, nodes[i]->decision);
    }
}

TEST(BinConsensus, JustWorks) {
    auto gen_one = [](uint32_t) { return 1;};
    auto gen_mod = [](uint32_t id) { return id % 2;};
    auto gen_rand = [](uint32_t) { return std::rand() % 2;};

    for (int n = 4; n < 20; ++n) {
        sanity_check(n, gen_one);
        sanity_check(n, gen_mod);
        sanity_check(n, gen_rand);
    }
}


void shuffle_check(int n, std::function<uint32_t(uint32_t)> gen_proposal) {
    ManualNetwork net;
    std::vector<std::unique_ptr<BinaryNode>> nodes;

    for (int i = 0; i < n; ++i) {
        nodes.emplace_back(std::make_unique<BinaryNode>(i, net));
    }
    for (int i = 0; i < n; ++i) {
        nodes[i]->run(gen_proposal(i));
    }

    for (int i = 0; i < 20; ++i) {
        std::this_thread::sleep_for(20ms);
        net.shuffle();
        net.shuffle();
        net.shuffle();
        net.step_n(n * n * 5);
    }

    DVLOG(4) << "TEST BinConsensus.MixedOrder: all shuffles are done" << std::endl;
    net.run_detached();

    for (int i = 0; i < n; ++i) {
        nodes[i]->join();
    }

    net.shutdown();

    for (int i = 1; i < n; ++i) {
        EXPECT_EQ(nodes[0]->decision, nodes[i]->decision);
    }
}

TEST(BinConsensus, MixedOrder) {
    auto gen_one = [](uint32_t) { return 1;};
    auto gen_mod = [](uint32_t id) { return id % 2;};
    auto gen_rand = [](uint32_t) { return std::rand() % 2;};

    for (int n = 4; n < 22; ++n) {
        shuffle_check(n, gen_one);
        shuffle_check(n, gen_mod);
        shuffle_check(n, gen_rand);
        VLOG(1) << "TEST BinConsensus.MixedOrder nodes: " << n << std::endl;
    }
}

int main(int argc, char **argv) {
    // FLAGS_log_dir = "./log";
    FLAGS_v = -1;
    // google::InitGoogleLogging(argv[0]);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}