#include "DBFT.hpp"
#include "BinConsensus.hpp"

DBFT::DBFT(uint32_t block_id, uint32_t nodes_cnt, uint32_t batch_size, INetManager& net, TransactionPool& pool, Role role) 
    : block_id_(block_id),
      nodes_cnt_(nodes_cnt),
      batch_size_(batch_size),
      net_(net),
      role_(role),
      proposals_(batch_size_ * nodes_cnt_), 
      RB_(nodes_cnt, net),
      decision_(nodes_cnt_),
      ready_(nodes_cnt_),
      invoked_(nodes_cnt_) {
        // start_time_ = std::chrono::system_clock::now();

        for (size_t i = 0; i < nodes_cnt_; ++i) {
            bin_cons_.emplace_back(nodes_cnt, net, json{{"block_id", block_id_}, {"bin_con_id", i}});
        }

        if (role != Fair) {
            execute_byzantine(role);
            // return;
        }

        std::vector<Transaction> tx;
        for (size_t i = 0; i < batch_size_; ++i) {
            tx.push_back(pool.get_tx());
        }
        proposals_[net_.get_id()] = tx;

        json data;
        data["block_id"] = block_id_;
        data["transactions"] = tx;
        data["index"] = net_.get_id();
        DVLOG(3) << net_.get_id() << " DBFT RB: " << data << std::endl;
        RB_.broadcast(data);
        state_ = AwaitProposals;
}

// DBFT::DBFT(uint32_t nodes_cnt, INetManager& net, Transaction tx)
//     : nodes_cnt_(nodes_cnt), proposals_(1, tx),
//       RB_(nodes_cnt, net), bin_cons_(MAX_BATCH, BinConsensus(nodes_cnt, net)) {
//     json data = {{"transaction", tx}, {"from", net.get_id()}};
//     RB_.broadcast(data);
// }

bool DBFT::process_msg(Message msg) {
    if (state_ == Consensus) {
        return true;
    }

    if (!msg.data.contains("block_id") || msg.data["block_id"] != block_id_) {
        return false;
    }

    if (!msg.data.contains("bin_con_id")) {
        if (process_await_proposals(msg)) {
            check_if_consensus();
        }
        return state_ == Consensus;
    }

    size_t index = msg.data["bin_con_id"];
    assert(index >= 0 && index < nodes_cnt_);
    if (ready_[index]) {
        return false;
    }

    if (msg.type != "BV" && !msg.type.starts_with("BinConsensus_")) {
        return false;
    }

    if (bin_cons_[index].process_msg(msg)) {
        ready_[index] = 1;
        decision_[index] = bin_cons_[index].get_decision();
        uint32_t fail = (nodes_cnt_ - 1) / 3;
        if (!invoked_.all() && ready_.count() >= nodes_cnt_ - fail) {
            for (size_t i = 0; i < bin_cons_.size(); ++i) {
                bin_cons_[i].bin_propose(0);
                invoked_[i] = 1;
            }
        }
        check_if_consensus();
    }

    DVLOG(3) << net_.get_id() << " DBFT block_id: " << block_id_ << " state_: " << state_
               << " invoked_:" << invoked_ << " ready_: " << ready_ << std::endl;
    return state_ == Consensus;
}

void DBFT::check_if_consensus() {
    if (role_ != Fair) {
        for (size_t i = 0; i < bin_cons_.size(); ++i) {
            if (proposals_[i].empty()) {
                return;
            }
        }
        state_ = Consensus;
        return;
    }

    if (ready_.all()) {
        for (size_t i = 0; i < bin_cons_.size(); ++i) { // change to decision_ & received_ == decision_
            if (decision_[i] == 1 && proposals_[i].empty()) {
                return;
            }
        }
        state_ = Consensus;
        set_metrics();
        DVLOG(2) << net_.get_id() << " DBFT CONSENSUS block_id: " << block_id_ << std::endl;
    }
}

bool DBFT::process_await_proposals(Message msg) {
    if (!msg.type.starts_with("RB_")) {
        return false;
    }

    auto RB_res = RB_.process_msg(std::move(msg));
    if (!RB_res.has_value()) {
        return false;
    }

    // BV-delivery
    // add to bin_values_[round_] upon BV-delivery
    json delivered_data = RB_res.value();
    if (!delivered_data.contains("block_id") || delivered_data["block_id"] != block_id_
        || !delivered_data.contains("transactions") || !delivered_data.contains("index")) {
        return false;
    }

    size_t index = delivered_data["index"];
    std::vector<Transaction> tx = delivered_data["transactions"];
    assert(index < nodes_cnt_ && !tx.empty());
    proposals_[index] = tx;
    bin_cons_[index].bin_propose(1);
    invoked_[index] = 1;

    uint32_t fail = (nodes_cnt_ - 1) / 3;
    if (state_ == AwaitProposals && ready_.count() >= nodes_cnt_ - fail) {
        for (size_t i = 0; i < bin_cons_.size(); ++i) {
            bin_cons_[i].bin_propose(0);
            invoked_[i] = 1;
        }
        state_ = AwaitBinCons;
    }

    return true;
}

Block DBFT::get_block(Chain& chain) {
    assert(state_ == Consensus);

    Block block;
    for (size_t i = 0; i < bin_cons_.size(); ++i) {
        if (decision_[i] == 1) {
            std::vector<Transaction> tx_vec = proposals_[i];
            for (auto tx : tx_vec) {
                if (!block.conflicts(tx) && !chain.conflicts(tx)) {
                    block.data.push_back(tx);
                }
            }
        }
    }

    return block;
}

ConsensusMetrics DBFT::get_metrics() {
    assert(state_ == Consensus);
    if (res_metrics_.rounds_number == 0) {
        set_rounds_number();
    }
    return res_metrics_;
}

void DBFT::set_metrics() {
    assert(state_ == Consensus);
    res_metrics_.block_size = decision_.count() * batch_size_;
}

void DBFT::set_rounds_number() {
    assert(state_ == Consensus);
    for (size_t i = 0; i < bin_cons_.size(); ++i) {
        res_metrics_.rounds_number = std::max(
            res_metrics_.rounds_number, 
            bin_cons_[i].get_metrics().rounds_number
        );
    }
}

void DBFT::execute_byzantine(Role role) {
    for (size_t i = 0; i < bin_cons_.size(); ++i) {
        bin_cons_[i].execute_byzantine(role);
        invoked_[i] = 1;
    }
}
