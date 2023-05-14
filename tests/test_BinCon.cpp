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
#include <../src/consensus/ReliableBroadcast.hpp>
#include <../src/consensus/BinConsensus.hpp>
#include <../src/node/node.hpp>
#include <../src/simulation/simulation.hpp>

using namespace std::chrono_literals;

// std::ofstream result_file("../tests/test_results/PSync_result.csv", std::ios::out);


void sanity_check(int n, std::function<uint32_t(uint32_t)> gen_proposal) {
    ManualNetwork net;
    std::vector<std::unique_ptr<BinaryNode>> nodes;

    for (int i = 0; i < n; ++i) {
        nodes.emplace_back(std::make_unique<BinaryNode>(i, net, Fair, gen_proposal(i)));
    }
    for (int i = 0; i < n; ++i) {
        nodes[i]->run();
    }

    net.run_detached();

    for (int i = 0; i < n; ++i) {
        nodes[i]->join();
    }

    net.shutdown();

    for (int i = 1; i < n; ++i) {
        EXPECT_EQ(nodes[0]->get_metrics().decision, nodes[i]->get_metrics().decision);
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
        nodes.emplace_back(std::make_unique<BinaryNode>(i, net, Fair, gen_proposal(i)));
    }
    for (int i = 0; i < n; ++i) {
        nodes[i]->run();
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
        EXPECT_EQ(nodes[0]->get_metrics().decision, nodes[i]->get_metrics().decision);
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

void timer_network_check(int n, std::function<uint32_t(uint32_t)> gen_proposal) {
    TimerNetwork net;
    std::vector<std::unique_ptr<BinaryNode>> nodes;

    for (int i = 0; i < n; ++i) {
        nodes.emplace_back(std::make_unique<BinaryNode>(i, net, Fair, gen_proposal(i)));
    }
    for (int i = 0; i < n; ++i) {
        nodes[i]->run();
    }

    for (int i = 0; i < n; ++i) {
        nodes[i]->join();
    }

    net.shutdown();

    for (int i = 1; i < n; ++i) {
        EXPECT_EQ(nodes[0]->get_metrics().decision, nodes[i]->get_metrics().decision);
    }
}

TEST(BinConsensus, TimerNetwork) {
    auto gen_one = [](uint32_t) { return 1;};
    auto gen_mod = [](uint32_t id) { return id % 2;};
    auto gen_rand = [](uint32_t) { return std::rand() % 2;};

    for (int n = 4; n < 20; ++n) {
        timer_network_check(n, gen_one);
        timer_network_check(n, gen_mod);
        timer_network_check(n, gen_rand);
    }
}

// void run_simulation(std::string sim_type, size_t n, size_t f, Role fail_role, size_t run_id) {
//     TimerNetwork net;
//     BinarySimulationConfig config = {net, sim_type, n, f, fail_role};

//     BinarySimulation sim(config);

//     sim.run();
//     sim.join();

//     net.shutdown();

//     sim.write_results(result_file, run_id);
// }

// TEST(BinCon, SimulationOk) {
//     for (size_t n = 4; n <= 31; n += 3) {
//         for (size_t i = 0; i < 100; ++i) {
//             run_simulation("Ok", n, 0, Fair, i);
//         }
//     }
// }

// TEST(BinCon, SimulationFail) {
//     for (size_t n = 4; n <= 31; n += 3) {
//         for (size_t i = 0; i < 100; ++i) {
//             run_simulation("FailStop", n, (n - 1) / 3, FailStop, i);
//             run_simulation("TxRejector", n, (n - 1) / 3, TxRejector, i);
//             run_simulation("BinConCrasher", n, (n - 1) / 3, BinConCrasher, i);
//         }
//     }
// }

int main(int argc, char **argv) {
    // FLAGS_log_dir = "./log";
    FLAGS_v = -1;
    // google::InitGoogleLogging(argv[0]);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}