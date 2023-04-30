#include "node.hpp"

void Node::run(size_t max_blocks) {
    net_manager_->update_nodes();
    std::thread thread([this, max_blocks]() {
        auto startTime = std::chrono::system_clock::now();

        DBFTs_.insert({
            0,
            DBFT(0, net_manager_->get_nodes_cnt(), *net_manager_, pool_)
        });

        std::optional<Message> msg_opt;
        while ((msg_opt = net_manager_->receive()) != std::nullopt) {
            auto msg = msg_opt.value();

            if (!msg.data.contains("block_id")) {
                continue;
            }

            size_t block_id = msg.data["block_id"];
            if (block_id < chain_.get_height()) {
                continue;
            }

            if (DBFTs_.find(block_id) == DBFTs_.end()) {
                DBFTs_.insert({
                    block_id,
                    DBFT(block_id, net_manager_->get_nodes_cnt(), *net_manager_, pool_)
                });
            }

            if (DBFTs_.at(block_id).process_msg(msg)) {
                Block new_block = DBFTs_.at(block_id).get_block(chain_);
                chain_.add_block(new_block);
                if (chain_.get_height() == max_blocks) {
                    break;
                }

                DLOG(INFO) << id_ << " BLOCK #" << block_id << ": " << new_block << std::endl;

                if (!DBFTs_.contains(block_id + 1)) {
                    DBFTs_.insert({
                        block_id + 1,
                        DBFT(block_id + 1, net_manager_->get_nodes_cnt(), *net_manager_, pool_)
                    });
                }
            }
        }

        auto endTime = std::chrono::system_clock::now();
        runtime_ = std::chrono::duration<double>(endTime - startTime).count();
    });

    msg_processor_ = std::move(thread);
}