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


std::ofstream result_fail("../tests/test_results/Fail_result.csv", std::ios::out);
// std::ofstream result_16("../tests/test_results/16_result.csv", std::ios::out);


void run_simulation(SimulationConfig& config, std::ofstream& file, size_t run_id) {
    TimerNetwork net;

    Simulation sim(net, config);

    sim.run();
    sim.join();

    net.shutdown();

    sim.write_results(file, run_id);
}

TEST(DBFT, Simulation) {
    std::ofstream result_file("../tests/test_results/Ok_result.csv", std::ios::out);

    for (size_t n = 4; n <= 31; n += 3) {
        SimulationConfig config = {
            "Ok",
            n,
            0,
            true,
            {1, 10, Fair}
        };

        for (size_t i = 0; i < 20; ++i) {
            run_simulation(config, result_file, i);
        }
    }
}

TEST(DBFT, SimulationBatch) {
    std::ofstream result_file("../tests/test_results/Batch_result.csv", std::ios::out);

    size_t n = 16;
    for (size_t batch : {1, 10, 20, 50, 100, 200, 500, 1000}) {
        SimulationConfig config = {
            "Batch",
            n,
            0,
            true,
            {1, batch, Fair}
        };

        for (size_t i = 0; i < 20; ++i) {
            run_simulation(config, result_file, i);
        }
    }
}

TEST(DBFT, FailStop) {
    for (size_t n = 4; n <= 31; n += 3) {
        SimulationConfig config = {
            "FailStop",
            n,
            (n - 1) / 3,
            true,
            {1, 10, FailStop}
        };

        for (size_t i = 0; i < 20; ++i) {
            run_simulation(config, result_fail, i);
        }
    }
}

TEST(DBFT, Rejectors) {
    for (size_t n = 4; n <= 31; n += 3) {
        SimulationConfig config = {
            "Rejector",
            n,
            (n - 1) / 3,
            false,
            {1, 10, TxRejector, true}
        };

        for (size_t i = 0; i < 20; ++i) {
            run_simulation(config, result_fail, i);
        }
    }
}

TEST(DBFT, BinConCrashers) {
    for (size_t n = 4; n <= 31; n += 3) {
        SimulationConfig config = {
            "BinConCrasher",
            n,
            (n - 1) / 3,
            false,
            {1, 10, BinConCrasher, true}
        };

        for (size_t i = 0; i < 20; ++i) {
            run_simulation(config, result_fail, i);
        }
    }
}

// TEST(DBFT, TxRejectors) {
//     size_t n = 16;
//     for (size_t f = 0; f <= (n - 1) / 3; ++f) {
//         for (size_t i = 0; i < 20; ++i) {
//             run_simulation("Rejector", n, f, 10, TxRejector, i);
//         }
//     }
// }

// TEST(DBFT, BinConCrashers) {
//     size_t n = 16;
//     for (size_t f = 0; f <= (n - 1) / 3; ++f) {
//         for (size_t i = 0; i < 20; ++i) {
//             run_simulation("BinConCrasher", n, f, 10, BinConCrasher, i);
//         }
//     }
// }

// TEST(DBFT, SimulationFailStop) {
//     size_t n = 16;
//     for (size_t f = 0; f <= (n - 1) / 3; ++f) {
//         for (size_t i = 0; i < 20; ++i) {
//             run_simulation("FailStop", n, f, 10, FailStop, i);
//         }
//     }
// }

int main(int argc, char **argv) {
    // FLAGS_log_dir = "./log";
    FLAGS_v = -1;
    FLAGS_minloglevel = -1;
    // google::InitGoogleLogging(argv[0]);
    // google::SetCommandLineOption("GLOG_minloglevel", "4");

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}