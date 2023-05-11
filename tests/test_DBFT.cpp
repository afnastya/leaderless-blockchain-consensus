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


std::ofstream result_file("../tests/test_results/DBFT_result.csv", std::ios::app);

void sanity_check(size_t n, size_t f = 0, size_t batch_size = 5) {
    TimerNetwork net;
    SimulationConfig config = {net, Ok, n, f, {3, batch_size}};

    Simulation sim(config);

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
    size_t n = 10;
    for (size_t batch = 1; batch < 32; batch += 5) {
        sanity_check(n, 3, batch);
    }
}

TEST(DBFT, FailStop) {
    for (size_t n = 4; n < 16; ++n) {
        for (size_t failstop_cnt = 0; failstop_cnt <= (n - 1) / 3; ++failstop_cnt) {
            sanity_check(n, failstop_cnt);
            DLOG(INFO) << "TEST DBFT.JustWorks nodes: " << n << " failstop: " << failstop_cnt << std::endl;
        }
    }
}

void run_simulation(SimulationType sim_type, size_t n, size_t f, size_t batch_size, size_t run_id) {
    TimerNetwork net;
    SimulationConfig config = {net, sim_type, n, f, {1, batch_size}};

    Simulation sim(config);

    sim.run();
    sim.join();

    net.shutdown();

    sim.write_results(result_file, run_id);
}

TEST(DBFT, Simulation) {
    for (size_t n = 4; n <= 16; n += 3) {
        for (size_t i = 0; i < 10; ++i) {
            run_simulation(Ok, n, 0, 1, i);
        }
    }
}

TEST(DBFT, SimulationBatch) {
    size_t n = 16;
    for (size_t batch = 5; batch <= 45; batch += 5) {
        for (size_t i = 0; i < 10; ++i) {
            run_simulation(Batch, n, 0, batch, i);
        }
    }
}

TEST(DBFT, SimulationFailStop) {
    size_t n = 16;
    for (size_t f = 0; f <= (n - 1) / 3; ++f) {
        for (size_t i = 0; i < 10; ++i) {
            run_simulation(FailStop, n, f, 1, i);
        }
    }
}

int main(int argc, char **argv) {
    // FLAGS_log_dir = "./log";
    FLAGS_v = -1;
    // google::InitGoogleLogging(argv[0]);
    // google::SetCommandLineOption("GLOG_minloglevel", "4");

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}