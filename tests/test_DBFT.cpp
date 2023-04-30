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
#include <../src/consensus/DBFT.hpp>
#include <../src/node/node.hpp>

void sanity_check(size_t n, size_t failstop_cnt = 0) {
    ManualNetwork net;
    std::vector<INode*> nodes;
    for (size_t i = 0; i + failstop_cnt < n; ++i) {
        nodes.emplace_back(new Node(i, net));
    }

    for (size_t i = nodes.size(); i < n; ++i) {
        nodes.emplace_back(new FailStopNode(i, net));
    }

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < 5; ++j) {
            nodes[i]->add_tx(i * 5 + j);
        }
    }

    for (size_t i = 0; i < n; ++i) {
        nodes[i]->run(3);
    }

    net.run_detached();

    for (size_t i = 0; i < n; ++i) {
        nodes[i]->join();
    }

    net.shutdown();

    Node* expected_node = dynamic_cast<Node*>(nodes[0]);   
    for (size_t i = 1; i < n; ++i) {
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

TEST(DBFT, FailStop) {
    for (size_t n = 4; n < 16; ++n) {
        for (size_t failstop_cnt = 0; failstop_cnt <= (n - 1) / 3; ++failstop_cnt) {
            sanity_check(n, failstop_cnt);
            DLOG(INFO) << "TEST DBFT.JustWorks nodes: " << n << " failstop: " << failstop_cnt << std::endl;
        }
    }
}

double run_simulation(size_t n, size_t failstop_cnt = 0) {
    ManualNetwork net;
    std::vector<INode*> nodes;
    for (size_t i = 0; i + failstop_cnt < n; ++i) {
        nodes.emplace_back(new Node(i, net));
    }

    for (size_t i = nodes.size(); i < n; ++i) {
        nodes.emplace_back(new FailStopNode(i, net));
    }

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < 5; ++j) {
            nodes[i]->add_tx(i * 5 + j);
        }
    }

    for (size_t i = 0; i < n; ++i) {
        nodes[i]->run(1);
    }

    net.run_detached();

    for (size_t i = 0; i < n; ++i) {
        nodes[i]->join();
    }

    net.shutdown();

    Node* expected_node = dynamic_cast<Node*>(nodes[0]);
    double runtime_sum = 0;
    for (size_t i = 1; i < n; ++i) {
        Node* node = dynamic_cast<Node*>(nodes[i]);
        if (!node) {
            continue;
        }

        runtime_sum += node->get_runtime();
    }

    return runtime_sum / (n - failstop_cnt); 
}

TEST(DBFT, Simulation) {
    for (size_t n = 4; n <= 16; n += 3) {
        for (size_t i = 0; i < 100; ++i) {
            double time = run_simulation(n, 0);
            std::cout << time << " ";
        }
        std::cout << std::endl;
    }
}

TEST(DBFT, SimulationFailStop) {
    size_t n = 16;
    for (size_t t = 0; t <= 5; ++t) {
        for (size_t i = 0; i < 100; ++i) {
            double time = run_simulation(n, t);
            std::cout << time << " ";
        }
        std::cout << std::endl;
    }
}

int main(int argc, char **argv) {
    FLAGS_log_dir = "./log";
    google::InitGoogleLogging(argv[0]);
    // google::SetCommandLineOption("GLOG_minloglevel", "4");

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}