#pragma once

struct ConsensusMetrics {
    // double runtime{0};      // unused
    size_t block_size{0};

    /* max rounds_number of all BinConsensuses
       insize Consensus Algorithm */
    size_t rounds_number{0};
};

struct BinConsensusMetrics {
    // double runtime{0};      // unused
    bool decision{false};
    size_t rounds_number{0};
};