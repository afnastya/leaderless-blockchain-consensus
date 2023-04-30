#include "BinConsensus.hpp"

#include <iostream>
#include <glog/logging.h>


BinConsensus::BinConsensus(uint32_t nodes_cnt, INetManager& net, json msg_base)
    : nodes_cnt_(nodes_cnt), net_(net), msg_base_(msg_base), RB_(nodes_cnt, net) {
    state_ = Uninvoked;
}


void BinConsensus::bin_propose(uint32_t value) {
    if (state_ != Uninvoked) {
        return;
    }

    state_ = Init;
    round_ = 0;
    est_ = value;
    if (rounds_.empty()) {
        rounds_.emplace_back();
    }

    phase_1();
}


bool BinConsensus::process_msg(Message msg) {
    if (state_ == Consensus) {
        return true;
    }

    if (msg.type.starts_with("RB_")) {
        process_bv_broadcast(msg);
    } else if (msg.type == "BinConsensus_AUX") {
        process_AUX(msg);
    }

    continue_if_ready();

    DLOG_IF(INFO, state_ == Consensus) << net_.get_id() << " CONSENSUS round: " 
                                      << round_ << " decision: " << get_decision() << std::endl;

    return state_ == Consensus;
}

bool BinConsensus::get_decision() {
    assert(state_ == Consensus);
    return est_;
}


void BinConsensus::inc_round(uint32_t new_est) {
    ++round_;
    est_ = new_est;
    if (rounds_.size() <= round_) {
        rounds_.emplace_back();
    }
}


void BinConsensus::add_binvalue(uint32_t round, uint32_t value) {
    if (value > 3) {
        return;
    }

    if (rounds_.size() <= round) {
        rounds_.resize(round + 1);
    }

    BinValues& bin_values = rounds_[round].bin_values;

    if (bin_values == Both || bin_values == static_cast<BinValues>(value + 1)) {
        return;
    }

    bin_values = static_cast<BinValues>(bin_values + value + 1);
}


void BinConsensus::phase_1() {
    json EST_data(msg_base_);
    EST_data["author"] = net_.get_id();
    EST_data["round"] = round_;
    EST_data["value"] = est_;

    DLOG(INFO) << net_.get_id() << " phase_1 BvBroadcast EST: " << EST_data;

    RB_.broadcast(EST_data); // BV-broadcast est_;
    state_ = BvBroadcast;

    continue_if_ready();
}


void BinConsensus::process_bv_broadcast(Message msg) {
    DLOG(INFO) << net_.get_id() << " got msg: " << msg;

    if (!msg.data.contains("round") || msg.data["round"] < round_) {
        return;
    }

    auto RB_res = RB_.process_msg(std::move(msg));
    if (!RB_res.has_value()) {
        return;
    }

    // BV-delivery
    // add to bin_values_[round_] upon BV-delivery
    json delivered_data = RB_res.value();
    if (!delivered_data.contains("round") || delivered_data["round"] < round_
        || !delivered_data.contains("value")) {
        return;
    }

    add_binvalue(delivered_data["round"], delivered_data["value"]);
}


void BinConsensus::phase_2() {
    // broadcast AUX[ri](bin_values_[round_]);
    json AUX_data(msg_base_);
    AUX_data["round"] = round_;
    AUX_data["binvalues"] = rounds_[round_].bin_values;

    DLOG(INFO) << net_.get_id() << " phase_2 broadcast AUX: " << AUX_data;

    net_.broadcast(Message("BinConsensus_AUX", AUX_data));
    state_ = Broadcast;

    continue_if_ready();
}


void BinConsensus::process_AUX(Message msg) {
    DLOG(INFO) << net_.get_id() << " got msg: " << msg;

    if (msg.type != "BinConsensus_AUX") {
        return;
    }

    if (!msg.data.contains("round") || msg.data["round"] < round_
        || !msg.data.contains("binvalues")) {
        return;
    }

    uint32_t round = msg.data["round"];
    if (rounds_.size() <= round) {
        rounds_.resize(round + 1);
    }

    uint32_t bin_values = msg.data["binvalues"];
    rounds_[round].received_AUXes.insert(msg.from);
    rounds_[round].values |= bin_values;
}


void BinConsensus::phase_3(BinValues values) {
    uint32_t b = round_ % 2;

    DLOG(INFO) << net_.get_id() << " phase_3" << std::endl;

    if (values == Zero || values == One) {
        inc_round(values / 2);
        if (values / 2 == b) {
            state_ = Consensus;
            return;
        }
    } else {
        inc_round(b);
    }

    phase_1();
}


void BinConsensus::continue_if_ready() {
    auto& round = rounds_[round_];
    if (state_ == BvBroadcast && round.bin_values != None) {
        phase_2();
    } else if (state_ == Broadcast) {
        if (round.received_AUXes.size() < nodes_cnt_ - (nodes_cnt_ - 1) / 3) {
            return;
        }

        if (round.values != 0) { // make sure values in bin_values
            DLOG_IF(WARNING, (round.values & round.bin_values) != round.values) << net_.get_id()
                << " values " << round.values <<  " not in bin_values " << round.bin_values
                << "! round: " << round_ << std::endl;
            phase_3(static_cast<BinValues>(round.values & round.bin_values));
        }
    }
}