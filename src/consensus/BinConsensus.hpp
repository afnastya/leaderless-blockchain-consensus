#pragma once

#include <vector>
#include <unordered_set>

#include "../core/message.hpp"
#include "../network/netmanager.hpp"
#include "../consensus/ReliableBroadcast.hpp"

class BinConsensus {
private:
    enum State {
        Uninvoked,
        Init,
        BvBroadcast,
        Broadcast,
        Consensus,
    };

    enum BinValues {
        None = 0,
        Zero = 1,
        One = 2,
        Both = 3,
    };

    struct RoundData {
        BinValues bin_values{None};
        uint32_t values{0};
        std::unordered_set<uint32_t> received_AUXes;
    };

public:
    BinConsensus(uint32_t nodes_cnt, INetManager& net, json msg_base = {});
    void bin_propose(uint32_t value);
    bool process_msg(Message msg);
    bool get_decision();

private:
    void inc_round(uint32_t new_est);
    void add_binvalue(uint32_t round, uint32_t value);

    // process msg from BvBroadcast
    void process_bv_broadcast(Message msg);
    // process msg from broadcast
    void process_AUX(Message msg);

    void phase_1();
    void phase_2();
    void phase_3(BinValues values_);
    // function determines, whether algorithm can get to the next phase
    void continue_if_ready();

private:
    uint32_t nodes_cnt_; // why don't use net_.nodes_cnt()?
    INetManager& net_;
    json msg_base_; // info, how to get to this BinConsensus instance (block_id, bin_con_id)

    ReliableBroadcast RB_;
    State state_;
    uint32_t round_;
    uint32_t est_;
    std::vector<RoundData> rounds_;
};