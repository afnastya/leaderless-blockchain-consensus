#include "ReliableBroadcast.hpp"

ReliableBroadcast::ReliableBroadcast(int nodes_cnt, INetManager& net)
    : nodes_cnt_(nodes_cnt), net_(net) {
}

void ReliableBroadcast::broadcast(const json& data) {
    net_.broadcast(make_init(data));
    instances_.insert({data.dump(), {RB_INIT, {}}});
}

std::optional<json> ReliableBroadcast::process_msg(Message msg) {
    if (!check_type(msg)) {
        return std::nullopt;
    }

    Instance& instance = get_instance(msg.data);
    if (instance.state_ == RB_DELIVERED) {
        return std::nullopt;
    }

    if (msg.type == "RB_INIT" && instance.state_ == RB_INIT) {
        net_.broadcast(make_echo(msg.data));
        instance.state_ = RB_ECHO;
        return std::nullopt;
    }
    
    if (msg.type == "RB_ECHO") {
        instance.received_from_[1].insert(msg.from);
        if (instance.state_ < RB_READY && instance.received_from_[1].size() >= nodes_cnt_ - (nodes_cnt_ - 1) / 3) {
            net_.broadcast(make_ready(msg.data));
            instance.state_ = RB_READY;
        }
        return std::nullopt;
    }

    if (msg.type == "RB_READY") {
        instance.received_from_[2].insert(msg.from);
        if (instance.state_ < RB_READY && instance.received_from_[2].size() >= (nodes_cnt_ - 1) / 3 + 1) {
            net_.broadcast(make_ready(msg.data));
            instance.state_ = RB_READY;
        }

        if (instance.received_from_[2].size() >= nodes_cnt_ - (nodes_cnt_ - 1) / 3) {
            instance.state_ = RB_DELIVERED;
            // delete instance
            return msg.data;
        }
    }

    return std::nullopt;
}

bool ReliableBroadcast::is_delivered(const json& data) {
    Instance& instance = get_instance(data);
    return instance.state_ == RB_DELIVERED;
}

Message ReliableBroadcast::make_init(const json& data) {
    return Message("RB_INIT", data);
}

Message ReliableBroadcast::make_echo(const json& data) {
    return Message("RB_ECHO", data);
}

Message ReliableBroadcast::make_ready(const json& data) {
    return Message("RB_READY", data);
}

bool ReliableBroadcast::check_type(const Message& msg) const {
    const static std::vector<std::string> types = {"RB_INIT", "RB_ECHO", "RB_READY"};

    for (const auto& type : types) {
        if (msg.type == type) {
            return true;
        }
    }
    return false;
}

ReliableBroadcast::Instance& ReliableBroadcast::get_instance(const json& data) {
    if (!instances_.contains(data.dump())) {
        instances_.insert({data.dump(), {RB_INIT, {}}});
    }

    return instances_[data.dump()];
}