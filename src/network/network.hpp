#pragma once

#include <thread>
#include <vector>
#include <unordered_map>

#include "../core/message.hpp"
#include "channel.hpp"

class INetwork { // add run()
public:
    virtual IChannel& add_node(uint32_t node_id) = 0;
    virtual std::unordered_map<uint32_t, Sender> get_nodes() = 0;
    virtual ~INetwork() = default;
};


class ManualNetwork : public INetwork {
public:
    // Network();
    void run_detached();
    IChannel& add_node(uint32_t node_id) override;
    Sender get_net_sender();
    bool check_connection(const Message& msg) const;
    std::unordered_map<uint32_t, Sender> get_nodes() override;
    void shutdown();

    void shuffle(); // re-do
    bool step();
    size_t step_n(size_t cnt);
    void run();

private:
    Channel queue_;
    std::unordered_map<uint32_t, Channel> node_channels_;
    std::thread msg_processor_;

};


class Network : public INetwork {
public:
    IChannel& add_node(uint32_t node_id) override;
    std::unordered_map<uint32_t, Sender> get_nodes() override;
    void shutdown();

private:
    std::unordered_map<uint32_t, Channel> node_channels_;
};


class TimerNetwork : public INetwork {
public:
    IChannel& add_node(uint32_t node_id) override;
    std::unordered_map<uint32_t, Sender> get_nodes() override;
    void shutdown();

private:
    std::unordered_map<uint32_t, TimerChannel> node_channels_;
};