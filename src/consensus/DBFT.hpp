#pragma once

#include <vector>
#include <string>
#include <optional>
#include <boost/dynamic_bitset.hpp>

#include "../core/transaction.hpp"
#include "../core/chain.hpp"
#include "BinConsensus.hpp"
#include "ReliableBroadcast.hpp"


class DBFT {
private:
    enum State {
        AwaitProposals,
        AwaitBinCons,
        Consensus,
    };
public:
    DBFT(uint32_t block_id, uint32_t nodes_cnt, INetManager& net, TransactionPool& pool_);

    bool process_msg(Message msg);
    Block get_block(Chain& chain);

private:
    void process_await_proposals(Message msg);
    void check_if_consensus();
    

private:
    static const uint32_t MAX_BATCH = 1;

    uint32_t block_id_;
    uint32_t nodes_cnt_;
    INetManager& net_;
    std::vector<std::optional<Transaction>> proposals_;
    State state_;
    ReliableBroadcast RB_;
    std::vector<BinConsensus> bin_cons_;
    boost::dynamic_bitset<> decision_;
    boost::dynamic_bitset<> ready_;
    boost::dynamic_bitset<> invoked_;
};