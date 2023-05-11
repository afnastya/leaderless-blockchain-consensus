#pragma once

#include <unordered_set>

#include "../core/message.hpp"
#include "../network/netmanager.hpp"

class ReliableBroadcast {
private:
    enum State {
        RB_INIT = 0,
        RB_ECHO = 1,
        RB_READY = 2,
        RB_DELIVERED = 3,
    };

    struct Instance {
        State state_;
        std::unordered_set<uint32_t> received_from_[3];
    };

public:
    ReliableBroadcast(int nodes_cnt, INetManager& net);
    void broadcast(const json& data);
    std::optional<json> process_msg(Message msg);
    bool is_delivered(const json& data);

private:
    Message make_init(const json& data);
    Message make_echo(const json& data);
    Message make_ready(const json& data);

    bool check_type(const Message& msg) const;
    ReliableBroadcast::Instance& get_instance(const json& data);

private:
    uint32_t nodes_cnt_;
    INetManager& net_;
    std::unordered_map<std::string, Instance> instances_;
};