#pragma once

#include <thread>
#include <vector>
#include <unordered_map>

#include "../core/message.hpp"
#include "channel.hpp"

class INetwork {
public:
    virtual Receiver<Message> add_node(uint32_t node_id) = 0;
    virtual std::unordered_map<uint32_t, Sender<Message>> get_nodes() = 0;
    virtual ~INetwork() = default;
};


class ManualNetwork : public INetwork {
public:
    // Network();
    void run_detached();
    Receiver<Message> add_node(uint32_t node_id) override;
    Sender<Message> get_net_sender();
    bool check_connection(const Message& msg) const;
    std::unordered_map<uint32_t, Sender<Message>> get_nodes() override;
    void shutdown();

    void shuffle(); // re-do
    bool step();
    size_t step_n(size_t cnt);
    void run();

private:
    Channel<Message> queue_;
    std::unordered_map<uint32_t, Channel<Message>> node_channels_;
    std::thread msg_processor_;

};