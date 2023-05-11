#pragma once

struct ConsensusMetrics {
    double runtime{0};
    size_t block_size{0};
};

struct BinConsensusMetrics {
    double runtime{0};
    bool decision{false};
    size_t rounds_number{0};
};