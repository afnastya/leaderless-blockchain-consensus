#pragma once

#include <glog/logging.h>
#include <optional>

#include "../core/message.hpp"
#include "channel.hpp"
#include "network.hpp"

class INetManager {
public:
    virtual void update_nodes() = 0;
    virtual uint32_t get_nodes_cnt() const = 0;
    virtual uint32_t get_id() const = 0;
    virtual void send(Message msg) = 0;
    virtual std::optional<Message> receive() = 0;
    virtual void broadcast(Message msg) = 0;
    virtual void close() = 0;
    virtual ~INetManager() = default;
};


class NetManager : public INetManager {
public:
    NetManager(uint32_t id, Receiver<Message> receiver, INetwork& net) 
        : id_(id), receiver_(receiver), net_(net) {
    }

    void update_nodes() override {
        auto nodes = net_.get_nodes();
        DLOG(INFO) << id_ << " GET NODES: " << nodes.size() << std::endl;
        nodes_ = std::move(nodes);
    }

    uint32_t get_nodes_cnt() const override {
        return nodes_.size();
    }

    uint32_t get_id() const override {
        return id_;
    }

    void send(Message msg) override {
        if (!nodes_.contains(msg.to)) {
            return;
        }

        msg.from = id_;
        DLOG(INFO) << msg.from << " --> " << msg.to << " " << msg << std::endl;
        nodes_.at(msg.to).send(std::move(msg));
    }

    std::optional<Message> receive() override {
        std::optional<Message> msg_opt = receiver_.receive();
        if (msg_opt.has_value()) {
            Message msg = msg_opt.value();
            DLOG(INFO) << msg.to << " <-- " << msg.from << " " << msg << std::endl;
        }
        return msg_opt;
    }

    void broadcast(Message msg) override {
        // to all
        msg.from = id_;
        DLOG(INFO) << id_ << " nodes_.size(): " << nodes_.size() << std::endl;
        for (auto& node : nodes_) {
            msg.to = node.first;
            DLOG(INFO) << msg.from << " --> " << msg.to << " " << msg << std::endl;
            node.second.send(msg);
        }
    }

    void close() override {
        receiver_.close();
    }

private:
    uint32_t id_;
    Receiver<Message> receiver_;
    INetwork& net_;
    std::unordered_map<uint32_t, Sender<Message>> nodes_; // upgrade to map/vector of senders
};