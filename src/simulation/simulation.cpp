#include "simulation.hpp"

#include <string>
#include <unordered_map>


Simulation::Simulation(SimulationConfig& config) : config_(config) {
    assert(config_.fail * 3 < config_.nodes);
    generate_nodes();
    generate_tx();
}

Simulation::~Simulation() {
    for (size_t i = 0; i < nodes_.size(); ++i) {
        delete nodes_[i];
    }
}

void Simulation::run() {
    assert(!nodes_.empty());

    for (size_t i = 0; i < nodes_.size(); ++i) {
        nodes_[i]->run();
    }
}

void Simulation::join() {
    for (size_t i = 0; i < nodes_.size(); ++i) {
        nodes_[i]->join();
    }
}

std::vector<INode*>& Simulation::get_nodes() {
    return nodes_;
}

Node* Simulation::get_fair_node() {
    assert(!nodes_.empty());

    for (INode* node : nodes_) {
        Node* fair_node = dynamic_cast<Node*>(node);
        if (fair_node) {
            return fair_node;
        }
    }

    return nullptr;
}

void Simulation::write_results(std::ofstream& file, size_t run_id) {
    static const std::unordered_map<SimulationType, std::string> simtype2str 
        = {{Ok, "Ok"}, {Batch, "Batch"}, {FailStop, "FailStop"}};
    file << simtype2str.at(config_.sim_type) << ","
         << run_id << ","
         << config_.nodes << ","
         << config_.fail << ","
         << get_runtime() << ","
         << config_.sim_data.batch_size << ","
         << get_block_size() << "\n";
}


void Simulation::generate_nodes() {
    for (size_t i = 0; i + config_.fail < config_.nodes; ++i) {
        nodes_.emplace_back(new Node(i, config_.net, config_.sim_data));
    }

    for (size_t i = nodes_.size(); i < config_.nodes; ++i) {
        nodes_.emplace_back(new FailStopNode(i, config_.net, config_.sim_data));
    }

    // std::mt19937 engine(std::random_device{}());
    // std::shuffle(std::begin(nodes_), std::end(nodes_), engine);
}


void Simulation::generate_tx(size_t cnt) {
    assert(!nodes_.empty());

    for (size_t i = 0; i < cnt; ++i) {
        Transaction new_tx = gen_tx();
        generated_tx_.push_back(new_tx);

        size_t node_id = std::hash<Transaction>{}(new_tx) % nodes_.size();

        for (size_t j = 0; j <= get_max_failed(); ++j) {
            nodes_[node_id]->add_tx(new_tx);
            node_id = (node_id + 1) % nodes_.size();
        }
    }
}

double Simulation::get_runtime() {
    double runtime_sum = 0;
    for (size_t i = 1; i < config_.nodes; ++i) {
        Node* node = dynamic_cast<Node*>(nodes_[i]);
        if (!node) {
            continue;
        }

        runtime_sum += node->get_runtime();
    }

    return runtime_sum / (config_.nodes - config_.fail);
}

size_t Simulation::get_block_size() {
    Node* node = get_fair_node();
    return node->get_metrics().block_size;
}