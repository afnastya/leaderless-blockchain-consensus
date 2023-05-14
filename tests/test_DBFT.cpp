#include <gtest/gtest.h>
#include <glog/logging.h>
#include <iostream>

#include <deque>
#include <optional>
#include <functional>
#include <cstdlib>
#include <memory>
#include <fstream>

#include <../src/core/message.hpp>
#include <../src/network/netmanager.hpp>
#include <../src/network/network.hpp>
#include <../src/consensus/DBFT.hpp>
#include <../src/node/node.hpp>
#include <../src/simulation/simulation.hpp>


void sanity_check(size_t n, size_t f = 0, size_t batch_size = 5) {
    TimerNetwork net;
    SimulationConfig config = {"Ok", n, f, true, {3, batch_size}};

    Simulation sim(net, config);

    sim.run();
    sim.join();

    net.shutdown();

    Node* expected_node = sim.get_fair_node();
    auto nodes = sim.get_nodes(); 
    for (size_t i = 0; i < n; ++i) {
        Node* node = dynamic_cast<Node*>(nodes[i]);
        if (!node) {
            continue;
        }

        Chain& expected_chain = expected_node->get_chain();
        Chain& chain = node->get_chain();

        EXPECT_EQ(expected_chain.get_height(), chain.get_height());

        for (size_t j = 0; j < expected_node->get_chain().get_height(); ++j) {
            Block& expected_block = expected_chain.get_block(j);
            Block& block = chain.get_block(j);

            EXPECT_EQ(expected_block.data, block.data);
        }
    }
}

TEST(DBFT, JustWorks) {
    for (size_t n = 4; n < 16; ++n) {
        sanity_check(n, 0);
    }
}

TEST(DBFT, Batch) {
    size_t n = 16;
    for (size_t batch = 1; batch < 32; batch += 1) {
        sanity_check(n, 0, batch);
    }
}

TEST(DBFT, FailStop) {
    for (size_t n = 4; n < 16; ++n) {
        for (size_t failstop_cnt = 0; failstop_cnt <= (n - 1) / 3; ++failstop_cnt) {
            sanity_check(n, failstop_cnt);
        }
    }
}


int main(int argc, char **argv) {
    // FLAGS_log_dir = "./log";
    FLAGS_v = -1;
    FLAGS_minloglevel = -1;
    // google::InitGoogleLogging(argv[0]);
    // google::SetCommandLineOption("GLOG_minloglevel", "4");

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}