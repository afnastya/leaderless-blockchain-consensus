#pragma once
#include <glog/logging.h>

#include <deque>
#include <optional>

#include "../core/message.hpp"
#include "../network/netmanager.hpp"
#include "../network/network.hpp"
#include "../consensus/DBFT.hpp"
// #include "../src/logger.cpp"

class INode {
public:
    virtual void run(size_t max_blocks) = 0;
    virtual void join() = 0;
    virtual void add_tx(Transaction tx) = 0;
    virtual ~INode() = default;
};


class Node : public INode {
public:
    Node(uint32_t id, INetwork& net) : id_(id) {
        net_manager_ = new NetManager(id, net.add_node(id), net);
    }

    void run(size_t max_blocks);

    void add_tx(Transaction tx) {
        pool_.add_tx(tx);
    }

    Chain& get_chain() {
        return chain_;
    }

    void join() {
        msg_processor_.join();
    }

    uint32_t get_id() const {
        return id_;
    }

    double get_runtime() {
        assert(runtime_ != 0);
        return runtime_;
    }

    ~Node() {
        delete net_manager_;
    }

private:
    uint32_t id_;
    INetManager* net_manager_;
    std::thread msg_processor_;

    TransactionPool pool_;
    Chain chain_;
    std::unordered_map<uint32_t, DBFT> DBFTs_;
    double runtime_{0};
};

class FailStopNode : public INode {
public:
    FailStopNode(uint32_t id, INetwork& net) : id_(id) {
        net_manager_ = new NetManager(id, net.add_node(id), net);
    }

    void run(size_t) override {
        net_manager_->update_nodes();
        net_manager_->close();
    }

    void join() override {
    }

    void add_tx(Transaction tx) override {
        pool_.add_tx(tx);
    }

    ~FailStopNode() {
        delete net_manager_;
    }

private:
    uint32_t id_;
    INetManager* net_manager_;

    TransactionPool pool_;
};