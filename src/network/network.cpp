#include "network.hpp"

#include <algorithm>
#include <deque>
// #include "netmanager.hpp"

Receiver<Message> ManualNetwork::add_node(uint32_t node_id) {
    node_channels_[node_id];
    auto receiver = node_channels_[node_id].get_receiver();


    return receiver;
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

Sender<Message> ManualNetwork::get_net_sender() {
    return queue_.get_sender();
}

std::unordered_map<uint32_t, Sender<Message>> ManualNetwork::get_nodes() {
    std::unordered_map<uint32_t, Sender<Message>> nodes;
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
    static Receiver<Message> receiver_ = queue_.get_receiver();;
    if (receiver_.size() == 0) {
        return false;
    }

    Message msg = receiver_.receive().value();
    if (check_connection(msg)) {
        node_channels_[msg.to].get_sender().send(std::move(msg));
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
    auto receiver = queue_.get_receiver();
    std::optional<Message> opt_msg;
    while ((opt_msg = receiver.receive()) != std::nullopt) {
        Message msg = opt_msg.value();
        if (!check_connection(msg)) {
            continue;
        }

        node_channels_[msg.to].get_sender().send(std::move(msg));
    }
}

void ManualNetwork::shuffle() {
    std::deque<Message> extracted;
    auto receiver = queue_.get_receiver();

    for (size_t i = 0; i < receiver.size(); ++i) {
        Message msg = receiver.receive().value();
        extracted.push_back(std::move(msg));
    }

    auto engine = std::default_random_engine{};
    std::shuffle(std::begin(extracted), std::end(extracted), engine);

    auto sender = queue_.get_sender();
    while (!extracted.empty()) {
        sender.send(std::move(extracted.front()));
        extracted.pop_front();
    }
}