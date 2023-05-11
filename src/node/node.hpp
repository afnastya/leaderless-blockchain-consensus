#pragma once
#include <glog/logging.h>

#include <deque>
#include <optional>

#include "../core/message.hpp"
#include "../network/netmanager.hpp"
#include "../network/network.hpp"
#include "../consensus/DBFT.hpp"
#include "../simulation/structs.hpp"

class INode {
public:
    virtual void run() = 0;
    virtual void join() = 0;
    virtual void add_tx(Transaction tx) = 0;
    virtual ~INode() = default;
};


class Node : public INode {
public:
    Node(uint32_t id, INetwork& net, SimulationData sim_data) : id_(id), sim_data_(sim_data) {
        auto handler = [this](Message msg) {
            this->handle_message(msg);
        };

        net_manager_ = new NetManager(id, net.add_node(id), net, handler);
    }

    void run() override;

    void handle_message(Message msg);

    void add_tx(Transaction tx) override {
        pool_.add_tx(tx);
    }

    Chain& get_chain() {
        return chain_;
    }

    void join() override {
        msg_processor_.join();
    }

    uint32_t get_id() const {
        return id_;
    }

    double get_runtime() {
        assert(metrics_.runtime != 0);
        return metrics_.runtime;
    }

    ConsensusMetrics get_metrics() {
        assert(metrics_.runtime != 0);
        return metrics_;
    }

    ~Node() override {
        delete net_manager_;
    }

private:
    uint32_t id_;
    INetManager* net_manager_;
    std::thread msg_processor_;

    TransactionPool pool_;
    Chain chain_;
    std::unordered_map<uint32_t, DBFT> DBFTs_;
    ConsensusMetrics metrics_;
    SimulationData sim_data_;
};

class FailStopNode : public INode {
public:
    FailStopNode(uint32_t id, INetwork& net, SimulationData sim_data) : id_(id), sim_data_(sim_data) {
        net_manager_ = new NetManager(id, net.add_node(id), net);
    }

    void run() override {
        net_manager_->update_nodes();
        net_manager_->close();
    }

    void join() override {
    }

    void add_tx(Transaction tx) override {
        pool_.add_tx(tx);
    }

    uint32_t get_id() const {
        return id_;
    }

    ~FailStopNode() {
        delete net_manager_;
    }

private:
    uint32_t id_;
    INetManager* net_manager_;

    TransactionPool pool_;

    SimulationData sim_data_;
};