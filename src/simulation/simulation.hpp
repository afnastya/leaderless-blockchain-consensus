#pragma once

#include <vector>
#include <functional>
#include <fstream>

#include "../core/transaction.hpp"
#include "../node/node.hpp"

class Simulation {
public:
    Simulation(SimulationConfig& config);
    ~Simulation();

    void run();
    void join();

    std::vector<INode*>& get_nodes();
    Node* get_fair_node();

    void write_results(std::ofstream& file, size_t run_id);

private:
    size_t get_max_failed() {
        assert(!nodes_.empty());
        return (nodes_.size() - 1) / 3;
    }

    void generate_nodes();
    void generate_tx(size_t cnt = TX_CNT);

    double get_runtime();
    size_t get_block_size();

private:
    static const size_t TX_CNT = 10000;

    SimulationConfig config_;
    std::vector<INode*> nodes_;
    std::vector<Transaction> generated_tx_;
};