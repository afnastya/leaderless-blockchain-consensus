#pragma once

#include <cstddef>
#include <glog/logging.h>
#include <optional>
#include <functional>

#include "../core/message.hpp"
#include "channel.hpp"
#include "network.hpp"

class INetManager {
public:
    virtual void update_nodes() = 0;
    virtual uint32_t get_nodes_cnt() const = 0;
    virtual uint32_t get_id() const = 0;
    virtual void send(Message msg) = 0;
    virtual void handle_messages() = 0;
    virtual void broadcast(Message msg) = 0;
    virtual void close() = 0;
    virtual void stop_receive() = 0;
    virtual void set_timer(uint32_t, std::function<void()>) = 0;
    virtual ~INetManager() = default;
};


class NetManager : public INetManager {
public:
    NetManager(
        uint32_t id,
        IChannel& channel,
        INetwork& net,
        std::function<void(Message)> handler = [](Message) {},
        bool net_invasion = false
    ) : id_(id), channel_(channel), net_(net), net_invasion_(net_invasion) {

        channel_.set_handler([handler = std::move(handler)](Message msg) {
            DVLOG(7) << msg.to << " <-- " << msg.from << " " << msg << std::endl;
            handler(std::move(msg));
        });
    }

    void update_nodes() override {
        auto nodes = net_.get_nodes();
        DVLOG(3) << id_ << " GET NODES: " << nodes.size() << std::endl;
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
        DVLOG(7) << msg.from << " --> " << msg.to << " " << msg << std::endl;
        nodes_.at(msg.to).send(std::move(msg));
    }

    std::optional<Message> receive() {
        std::optional<Message> msg_opt = channel_.receive();
        if (msg_opt.has_value()) {
            Message msg = msg_opt.value();
            DVLOG(7) << msg.to << " <-- " << msg.from << " " << msg << std::endl;
        }
        return msg_opt;
    }

    void handle_messages() override {
        if (net_invasion_) {
            channel_.set_invasion_data(true, nodes_.size());
        }
        channel_.handle_messages();
    }

    void stop_receive() override {
        channel_.stop();
    }

    void broadcast(Message msg) override {
        // to all
        msg.from = id_;
        for (auto& node : nodes_) {
            msg.to = node.first;
            DVLOG(7) << msg.from << " --> " << msg.to << " " << msg << std::endl;
            node.second.send(msg);
        }
    }

    void close() override {
        channel_.close();
    }

    void set_timer(uint32_t timeout, std::function<void()> handler) override {
        channel_.set_timer(timeout, handler);
    }

private:
    uint32_t id_;
    IChannel& channel_;
    INetwork& net_;
    bool net_invasion_;
    std::unordered_map<uint32_t, Sender> nodes_;
};