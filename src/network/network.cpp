#include "network.hpp"

#include <algorithm>
#include <deque>
// #include "netmanager.hpp"

///////////////////////////////////////     ManualNetwork     /////////////////////////////////////////////

IChannel& ManualNetwork::add_node(uint32_t node_id) {
    node_channels_[node_id];

    return node_channels_[node_id];
}


bool ManualNetwork::check_connection(const Message& msg) const {
    return node_channels_.contains(msg.to) && node_channels_.contains(msg.from);
    // check possible disconnection / net partitioning
}


void ManualNetwork::run_detached() {
    std::thread thread([this]() {
        this->run();
    });

    msg_processor_ = std::move(thread);
}

Sender ManualNetwork::get_net_sender() {
    return queue_.get_sender();
}

std::unordered_map<uint32_t, Sender> ManualNetwork::get_nodes() {
    std::unordered_map<uint32_t, Sender> nodes;
    for (const auto& node : node_channels_) {
        nodes.emplace(node.first, queue_.get_sender());
    }
    return nodes;
}

void ManualNetwork::shutdown() {
    queue_.close();
    for (auto& node : node_channels_) {
        node.second.close();
    }

    msg_processor_.join();
}

bool ManualNetwork::step() {
    if (queue_.size() == 0) {
        return false;
    }

    Message msg = queue_.receive().value();
    if (check_connection(msg)) {
        node_channels_[msg.to].send(std::move(msg));
    }

    return true;
}

size_t ManualNetwork::step_n(size_t cnt) {
    for (size_t i = 0; i < cnt; ++i) {
        if (!step()) {
            return i;
        }
    }

    return cnt;
}

void ManualNetwork::run() {
    std::optional<Message> opt_msg;
    while ((opt_msg = queue_.receive()) != std::nullopt) {
        Message msg = opt_msg.value();
        if (!check_connection(msg)) {
            continue;
        }

        node_channels_[msg.to].send(std::move(msg));
    }
}

void ManualNetwork::shuffle() {
    std::deque<Message> extracted;

    for (size_t i = 0; i < queue_.size(); ++i) {
        Message msg = queue_.receive().value();
        extracted.push_back(std::move(msg));
    }

    std::mt19937 engine(std::random_device{}());
    std::shuffle(std::begin(extracted), std::end(extracted), engine);

    while (!extracted.empty()) {
        queue_.send(std::move(extracted.front()));
        extracted.pop_front();
    }
}


///////////////////////////////////////     Network     //////////////////////////////////////////////

IChannel& Network::add_node(uint32_t node_id) {
    node_channels_[node_id];

    return node_channels_[node_id];
}

std::unordered_map<uint32_t, Sender> Network::get_nodes() {
    std::unordered_map<uint32_t, Sender> nodes;
    for (const auto& node : node_channels_) {
        nodes.emplace(node.first, node_channels_[node.first].get_sender());
    }
    return nodes;
}

void Network::shutdown() {
    for (auto& node : node_channels_) {
        node.second.close();
    }
}


/////////////////////////////////////     TimerNetwork     ///////////////////////////////////////////

IChannel& TimerNetwork::add_node(uint32_t node_id) {
    node_channels_[node_id];

    return node_channels_[node_id];
}

std::unordered_map<uint32_t, Sender> TimerNetwork::get_nodes() {
    std::unordered_map<uint32_t, Sender> nodes;
    for (const auto& node : node_channels_) {
        nodes.emplace(node.first, node_channels_[node.first].get_sender());
    }
    return nodes;
}

void TimerNetwork::shutdown() {
    for (auto& node : node_channels_) {
        node.second.close();
    }
}