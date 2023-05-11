#pragma once

#include "../network/network.hpp"

struct SimulationData {
    size_t max_blocks{1};
    size_t batch_size{1};
};

enum SimulationType {
    Ok,
    Batch,
    FailStop
};

struct SimulationConfig {
    INetwork& net;
    SimulationType sim_type;
    size_t nodes;
    size_t fail;
    SimulationData sim_data;
};
