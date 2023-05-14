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

        net_manager_ = new NetManager(id, net.add_node(id), net, handler, sim_data.net_invasion);
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
        assert(sim_data_.role == Fair);
        assert(runtime_ != 0);
        return runtime_;
    }

    ConsensusMetrics get_metrics() {
        assert(sim_data_.role == Fair);
        assert(runtime_ != 0);
        return metrics_;
    }

    bool is_fair() {
        return sim_data_.role == Fair;
    }

    ~Node() override {
        delete net_manager_;
    }

private:
    uint32_t id_;
    INetManager* net_manager_;
    std::thread msg_processor_;
    double runtime_;

    TransactionPool pool_;
    Chain chain_;
    std::unordered_map<uint32_t, DBFT> DBFTs_;
    ConsensusMetrics metrics_;
    SimulationData sim_data_;
};

// class FailStopNode : public INode {
// public:
//     FailStopNode(uint32_t id, INetwork& net, SimulationData sim_data) : id_(id), sim_data_(sim_data) {
//         net_manager_ = new NetManager(id, net.add_node(id), net);
//     }

//     void run() override {
//         net_manager_->update_nodes();
//         net_manager_->close();
//     }

//     void join() override {
//     }

//     void add_tx(Transaction tx) override {
//         pool_.add_tx(tx);
//     }

//     uint32_t get_id() const {
//         return id_;
//     }

//     ~FailStopNode() {
//         delete net_manager_;
//     }

// private:
//     uint32_t id_;
//     INetManager* net_manager_;

//     TransactionPool pool_;

//     SimulationData sim_data_;
// };

class BinaryNode : public INode {
public:
    BinaryNode(uint32_t id, INetwork& net, Role role, uint32_t proposal = 0)
        : id_(id), role_(role), proposal_(proposal) {
        auto handler = [this](Message msg) {
            this->handle_message(msg);
        };

        net_manager_ = new NetManager(id, net.add_node(id), net, handler);
    }

    void run() override {
        net_manager_->update_nodes();
        std::thread thread([this]() {
            auto startTime = std::chrono::system_clock::now();

            bin_con = new BinConsensus(net_manager_->get_nodes_cnt(), *net_manager_);

            
            if (is_fair()) {
                bin_con->bin_propose(proposal_);
                net_manager_->handle_messages();
            } else {
                bin_con->execute_byzantine(role_);
            }

            auto endTime = std::chrono::system_clock::now();
            runtime_ = std::chrono::duration<double>(endTime - startTime).count();
        });

        msg_processor_ = std::move(thread);
    }

    void handle_message(Message msg) {
        if (bin_con->process_msg(msg)) {
            metrics_ = bin_con->get_metrics();
            net_manager_->stop_receive();
        }
    }

    void join() override {
        msg_processor_.join();
    }

    void add_tx(Transaction) override {
    }

    uint32_t get_id() const {
        return id_;
    }

    bool is_fair() {
        return role_ == Fair;
    }

    double get_runtime() {
        assert(role_ == Fair);
        assert(runtime_ != 0);
        return runtime_;
    }

    BinConsensusMetrics get_metrics() {
        assert(role_ == Fair);
        assert(runtime_ != 0);
        return metrics_;
    }

    ~BinaryNode() {
        delete bin_con;
        delete net_manager_;
    }

private:
    uint32_t id_;
    INetManager* net_manager_;
    std::thread msg_processor_;

    double runtime_{0};
    BinConsensusMetrics metrics_;

    BinConsensus* bin_con{nullptr};
    Role role_{Fair};
    uint32_t proposal_;
};
