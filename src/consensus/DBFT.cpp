#include "DBFT.hpp"
#include "BinConsensus.hpp"

DBFT::DBFT(uint32_t block_id, uint32_t nodes_cnt, INetManager& net, TransactionPool& pool_) 
    : block_id_(block_id),
      nodes_cnt_(nodes_cnt),
      net_(net),
      proposals_(nodes_cnt_), 
      RB_(nodes_cnt, net),
      decision_(MAX_BATCH * nodes_cnt_),
      ready_(MAX_BATCH * nodes_cnt_),
      invoked_(MAX_BATCH * nodes_cnt_) {
        Transaction tx = pool_.get_tx(); // what if no tx???
        proposals_[net_.get_id()] = tx;

        for (size_t i = 0; i < nodes_cnt_; ++i) {
            bin_cons_.emplace_back(nodes_cnt, net, json{{"block_id", block_id_}, {"bin_con_id", i}});
        }

        json data;
        data["block_id"] = block_id_;
        data["transaction"] = tx;
        data["index"] = net_.get_id();
        DLOG(INFO) << net_.get_id() << " DBFT RB: " << data << std::endl;
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
    // LOG(INFO) << net_.get_id() << " DBFT GOT MESSAGE: " << msg << std::endl;
    if (state_ == Consensus) {
        return true;
    }

    if (!msg.data.contains("block_id") || msg.data["block_id"] != block_id_) {
        return false;
    }

    if (!msg.data.contains("bin_con_id")) {
        process_await_proposals(msg);
        check_if_consensus();
        return state_ == Consensus;
    }

    size_t index = msg.data["bin_con_id"];
    assert(index >= 0 && index < nodes_cnt_);
    if (msg.type.starts_with("RB_") || msg.type.starts_with("BinConsensus_")) {
        if (bin_cons_[index].process_msg(msg)) {
            ready_[index] = 1;
            decision_[index] = bin_cons_[index].get_decision();
            if (!invoked_.all()) {
                for (size_t i = 0; i < bin_cons_.size(); ++i) {
                    bin_cons_[i].bin_propose(0);
                    invoked_[index] = 1;
                }
            }
        }
    }

    check_if_consensus();
    DLOG(INFO) << net_.get_id() << " DBFT block_id: " << block_id_ << " state_: " << state_
               << " invoked_:" << invoked_ << " ready_: " << ready_ << std::endl;
    return state_ == Consensus;
}

void DBFT::check_if_consensus() {
    if (ready_.all()) {
        for (size_t i = 0; i < bin_cons_.size(); ++i) {
            if (decision_[i] == 1 && !proposals_[i].has_value()) {
                return;
            }
        }
        state_ = Consensus;
        DLOG(INFO) << net_.get_id() << " DBFT CONSENSUS block_id: " << block_id_ << std::endl;
    }
}

void DBFT::process_await_proposals(Message msg) {
    if (!msg.type.starts_with("RB_")) {
        return;
    }

    auto RB_res = RB_.process_msg(std::move(msg));
    if (!RB_res.has_value()) {
        return;
    }

    // BV-delivery
    // add to bin_values_[round_] upon BV-delivery
    json delivered_data = RB_res.value();
    if (!delivered_data.contains("block_id") || delivered_data["block_id"] != block_id_
        || !delivered_data.contains("transaction") || !delivered_data.contains("index")) {
        return;
    }

    size_t index = delivered_data["index"];
    Transaction tx = delivered_data["transaction"];
    assert(index < nodes_cnt_);
    proposals_[index] = tx;
    bin_cons_[index].bin_propose(1);
    invoked_[index] = 1;

    if (state_ == AwaitProposals && ready_.any()) { // what about (|P| - t) in Red Belly Blockchain? 
        for (size_t i = 0; i < bin_cons_.size(); ++i) {
            bin_cons_[i].bin_propose(0);
            invoked_[index] = 1;
        }
        state_ = AwaitBinCons;
    }
}

Block DBFT::get_block(Chain& chain) {
    assert(state_ == Consensus);

    Block block;
    for (size_t i = 0; i < bin_cons_.size(); ++i) {
        if (decision_[i] == 1) {
            Transaction tx = proposals_[i].value();
            if (!block.conflicts(tx) && !chain.conflicts(tx)) {
                block.data.push_back(tx);
            }
        }
    }

    return block;
}