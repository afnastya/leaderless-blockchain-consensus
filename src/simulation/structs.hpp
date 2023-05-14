#pragma once

#include "../network/network.hpp"
#include "../node/role.hpp"

struct SimulationData {
    size_t max_blocks{1};
    size_t batch_size{1};
    Role role{Fair};
    bool net_invasion{false};
};

struct SimulationConfig {
    std::string sim_type;
    size_t nodes;
    size_t fail;
    bool shuffle;
    SimulationData sim_data;
};

struct BinarySimulationConfig {
    INetwork& net;
    std::string sim_type;
    size_t nodes;
    size_t fail;
    Role fail_role;
};
