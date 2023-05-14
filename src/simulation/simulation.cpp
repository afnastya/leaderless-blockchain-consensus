#include "simulation.hpp"
#include "structs.hpp"

#include <string>
#include <unordered_map>


Simulation::Simulation(INetwork& net, SimulationConfig& config) : net_(net), config_(config) {
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
        if (fair_node && fair_node->is_fair()) {
            return fair_node;
        }
    }

    return nullptr;
}

void Simulation::write_results(std::ofstream& file, size_t run_id) {
    // static const std::unordered_map<SimulationType, std::string> simtype2str 
    //     = {{Ok, "Ok"}, {Batch, "Batch"}, {FailStop, "FailStop"}};

    file << "DBFT,"
         << config_.sim_type << ","
         << run_id << ","
         << config_.nodes << ","
         << config_.fail << ","
         << get_runtime() << ","
         << config_.sim_data.batch_size << ","
         << get_rounds_number() << ","
         << get_block_size() << "\n";
    file.flush();
}


void Simulation::generate_nodes() {
    SimulationData sim_data(config_.sim_data);
    for (size_t i = 0; i < config_.fail; ++i) {
        nodes_.emplace_back(new Node(i, net_, sim_data));
    }

    // LOG(INFO) << "config.sim_data.role: " << config_.sim_data.role
    //          << " f: " << config_.fail << std::endl;
    sim_data.role = Fair;
    for (size_t i = nodes_.size(); i < config_.nodes; ++i) {
        INode* node = new Node(i, net_, sim_data);
        nodes_.emplace_back(node);
    }

    if (config_.shuffle) {
        std::mt19937 engine(std::random_device{}());
        std::shuffle(std::begin(nodes_), std::end(nodes_), engine);
    }
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
        if (!node || !node->is_fair()) {
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

size_t Simulation::get_rounds_number() {
    Node* node = get_fair_node();
    return node->get_metrics().rounds_number;
}

// BinarySimulation::BinarySimulation(BinarySimulationConfig& config) : config_(config) {
//     assert(config_.fail * 3 < config_.nodes);
//     generate_nodes();
// }

// BinarySimulation::~BinarySimulation() {
//     for (size_t i = 0; i < nodes_.size(); ++i) {
//         delete nodes_[i];
//     }
// }

// void BinarySimulation::run() {
//     assert(!nodes_.empty());

//     for (size_t i = 0; i < nodes_.size(); ++i) {
//         nodes_[i]->run();
//     }
// }

// void BinarySimulation::join() {
//     for (size_t i = 0; i < nodes_.size(); ++i) {
//         nodes_[i]->join();
//     }
// }

// std::vector<BinaryNode*>& BinarySimulation::get_nodes() {
//     return nodes_;
// }

// BinaryNode* BinarySimulation::get_fair_node() {
//     assert(!nodes_.empty());

//     for (BinaryNode* node : nodes_) {
//         if (node->is_fair()) {
//             return node;
//         }
//     }

//     return nullptr;
// }

// void BinarySimulation::write_results(std::ofstream& file, size_t run_id) {
//     file << "PSync,"
//          << config_.sim_type << ","
//          << run_id << ","
//          << config_.nodes << ","
//          << config_.fail << ","
//          << get_runtime() << ","
//          << get_rounds_number() << "\n";
//     file.flush();
// }


// void BinarySimulation::generate_nodes() {
//     static auto gen_rand = [] { return std::rand() % 2;};
//     for (size_t i = 0; i + config_.fail < config_.nodes; ++i) {
//         nodes_.emplace_back(new BinaryNode(i, config_.net, Fair, gen_rand()));
//     }

//     for (size_t i = nodes_.size(); i < config_.nodes; ++i) {
//         nodes_.emplace_back(new BinaryNode(i, config_.net, config_.fail_role));
//     }

//     std::mt19937 engine(std::random_device{}());
//     std::shuffle(std::begin(nodes_), std::end(nodes_), engine);
// }

// double BinarySimulation::get_runtime() {
//     double runtime_sum = 0;
//     for (BinaryNode* node : nodes_) {
//         if (!node->is_fair()) {
//             continue;
//         }

//         runtime_sum += node->get_runtime();
//     }

//     return runtime_sum / (config_.nodes - config_.fail);
// }

// size_t BinarySimulation::get_rounds_number() {
//     size_t rounds = 0;
//     for (BinaryNode* node : nodes_) {
//         if (!node->is_fair()) {
//             continue;
//         }

//         rounds = std::max(rounds, node->get_metrics().rounds_number);
//     }

//     return rounds;
// }
