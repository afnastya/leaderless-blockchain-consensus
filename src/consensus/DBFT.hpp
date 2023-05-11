#pragma once

#include <vector>
#include <string>
#include <optional>
#include <boost/dynamic_bitset.hpp>

#include "../core/transaction.hpp"
#include "../core/chain.hpp"
#include "BinConsensus.hpp"
#include "ReliableBroadcast.hpp"
#include "metrics.hpp"


class DBFT {
private:
    enum State {
        AwaitProposals,
        AwaitBinCons,
        Consensus,
    };

public:
    DBFT(uint32_t block_id,
         uint32_t nodes_cnt,
         uint32_t batch_size,
         INetManager& net,
         TransactionPool& pool);

    bool process_msg(Message msg);
    Block get_block(Chain& chain);
    ConsensusMetrics get_metrics();

private:
    bool process_await_proposals(Message msg);
    void check_if_consensus();
    void set_metrics();

private:
    using TimePoint = std::chrono::time_point<
                        std::chrono::system_clock,
                        std::chrono::duration<long long, std::ratio<1, 1000000>>>;

    static const size_t MAX_BATCH = 10;

    uint32_t block_id_;
    uint32_t nodes_cnt_;
    uint32_t batch_size_{MAX_BATCH};
    INetManager& net_;

    TimePoint start_time_;
    ConsensusMetrics res_metrics_;
    std::vector<std::optional<Transaction>> proposals_;
    State state_;
    ReliableBroadcast RB_;
    std::vector<BinConsensus> bin_cons_;
    boost::dynamic_bitset<> decision_;
    boost::dynamic_bitset<> ready_;
    boost::dynamic_bitset<> invoked_;
};