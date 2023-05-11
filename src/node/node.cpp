#include "node.hpp"

void Node::run() {
    net_manager_->update_nodes();
    std::thread thread([this]() {
        auto startTime = std::chrono::system_clock::now();

        DBFTs_.insert({
            0,
            DBFT(0, net_manager_->get_nodes_cnt(), sim_data_.batch_size, *net_manager_, pool_)
        });

        net_manager_->handle_messages();

        auto endTime = std::chrono::system_clock::now();
        metrics_.runtime = std::chrono::duration<double>(endTime - startTime).count();
    });

    msg_processor_ = std::move(thread);
}

void Node::handle_message(Message msg) {
    if (!msg.data.contains("block_id")) {
        return;
    }

    size_t block_id = msg.data["block_id"];
    if (block_id < chain_.get_height()) {
        return;
    }

    if (DBFTs_.find(block_id) == DBFTs_.end()) {
        DBFTs_.insert({
            block_id,
            DBFT(block_id, net_manager_->get_nodes_cnt(), sim_data_.batch_size, *net_manager_, pool_)
        });
    }

    if (DBFTs_.at(block_id).process_msg(msg)) {
        Block new_block = DBFTs_.at(block_id).get_block(chain_);
        metrics_.block_size = new_block.size();
        chain_.add_block(new_block);
        DLOG(INFO) << id_ << " BLOCK #" << block_id << ": " << new_block << std::endl;

        if (chain_.get_height() >= sim_data_.max_blocks) {
            net_manager_->stop_receive();
            return;
        }

        if (!DBFTs_.contains(block_id + 1)) {
            DBFTs_.insert({
                block_id + 1,
                DBFT(block_id + 1, net_manager_->get_nodes_cnt(), sim_data_.batch_size, *net_manager_, pool_)
            });
        }
    }
}