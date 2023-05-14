#include "BinConsensus.hpp"

#include <iostream>
#include <glog/logging.h>


BinConsensus::BinConsensus(uint32_t nodes_cnt, INetManager& net, json msg_base, bool is_psync)
    : nodes_cnt_(nodes_cnt), net_(net), msg_base_(msg_base), BV_(nodes_cnt, net), is_psync_(is_psync) {
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

    if (msg.type == "BV") {
        process_bv_broadcast(msg);
    } else if (msg.type == "BinConsensus_AUX") {
        process_AUX(msg);
    } else if (msg.type == "BinConsensus_COORD" && is_psync_) {
        process_COORD(msg);
    }

    continue_if_ready();

    if (state_ == Consensus) {
        DVLOG(5) << net_.get_id() << " " << msg_base_ << " CONSENSUS round: " 
                 << res_metrics_.rounds_number << " decision: " << get_decision() << std::endl;
    }

    return state_ == Consensus;
}

bool BinConsensus::reached_consensus() {
    return state_ == Consensus;
}

bool BinConsensus::get_decision() {
    assert(decided_);
    return res_metrics_.decision;
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
    EST_data["round"] = round_;
    EST_data["value"] = est_;

    DVLOG(5) << net_.get_id() << " " << msg_base_ << " phase_1 BvBroadcast EST: " << EST_data;

    BV_.broadcast(EST_data); // BV-broadcast est_;
    state_ = BvBroadcast;
    if (decided_) {
        add_binvalue(round_, est_);
        rounds_[round_].timer_expired = true;
    }

    if (is_psync_ && !decided_) {
        net_.set_timer(10000 + 500 * (round_ + 1), [this]{
            this->rounds_[this->round_].timer_expired = true;
            this->continue_if_ready();
        });
    }

    continue_if_ready();
}


void BinConsensus::process_bv_broadcast(Message msg) {
    DVLOG(6) << net_.get_id() << " " << msg_base_ << " got msg: " << msg;

    if (!msg.data.contains("round") || msg.data["round"] < round_) {
        return;
    }

    auto BV_res = BV_.process_msg(std::move(msg));
    if (!BV_res.has_value()) {
        return;
    }

    // BV-delivery
    // add to bin_values_[round_] upon BV-delivery
    json delivered_data = BV_res.value();
    DVLOG(6) << net_.get_id() << " " << msg_base_ << " bv-delivered: " << delivered_data;
    if (!delivered_data.contains("round") || delivered_data["round"] < round_
        || !delivered_data.contains("value")) {
        return;
    }

    add_binvalue(delivered_data["round"], delivered_data["value"]);
}

void BinConsensus::process_COORD(Message msg) {
    DVLOG(6) << net_.get_id() << " " << msg_base_ << " got msg: " << msg;

    if (msg.type != "BinConsensus_COORD" || !is_psync_) {
        return;
    }

    if (!msg.data.contains("round") || msg.data["round"] < round_
        || !msg.data.contains("binvalues")) {
        return;
    }

    uint32_t round = msg.data["round"];
    if ((uint32_t)msg.from != round % nodes_cnt_) {
        // not real coordinator
        return;
    }

    if (rounds_.size() <= round) {
        rounds_.resize(round + 1);
    }

    uint32_t bin_values = msg.data["binvalues"];
    if (rounds_[round].coord == None && (bin_values == Zero || bin_values == One)) {
        rounds_[round].coord = static_cast<BinValues>(bin_values);
    }
}

void BinConsensus::phase_coord() {
    if (net_.get_id() != round_ % nodes_cnt_) {
        return;
    }

    if (rounds_[round_].coord != None) {
        // already broadcasted coordinator value
        return;
    }

    rounds_[round_].coord = rounds_[round_].bin_values;

    json COORD_data(msg_base_);
    COORD_data["round"] = round_;
    COORD_data["binvalues"] = rounds_[round_].coord;

    DVLOG(5) << net_.get_id() << " " << msg_base_ << " phase_coord broadcast COORD: " << COORD_data;

    net_.broadcast(Message("BinConsensus_COORD", COORD_data));
}

void BinConsensus::phase_2() {
    // broadcast AUX[ri](bin_values_[round_]);
    json AUX_data(msg_base_);
    AUX_data["round"] = round_;
    AUX_data["binvalues"] = rounds_[round_].bin_values;

    if (is_psync_ && !decided_) { 
        uint32_t coord = rounds_[round_].coord;
        if (coord != None && ((coord & rounds_[round_].bin_values) == coord)) {
            AUX_data["binvalues"] = rounds_[round_].coord;
        }
    }

    DVLOG(5) << net_.get_id() << " " << msg_base_ << " phase_2 broadcast AUX: " << AUX_data;

    net_.broadcast(Message("BinConsensus_AUX", AUX_data));
    state_ = Broadcast;

    if (decided_) {
        phase_3(static_cast<BinValues>(rounds_[round_].bin_values));
    } else {
        continue_if_ready();
    }
}


void BinConsensus::process_AUX(Message msg) {
    DVLOG(6) << net_.get_id() << " " << msg_base_ <<" got msg: " << msg;

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
    if (bin_values > BinValues::Both || bin_values == None) {
        return;
    }

    if (!rounds_[round].received_AUXes.contains(msg.from)) {
        rounds_[round].received_AUXes.insert(msg.from);
        ++rounds_[round].values[bin_values];
    }
}


void BinConsensus::phase_3(BinValues values) {
    int b = (round_ + 1) % 2;

    DVLOG(5) << net_.get_id() << " " << msg_base_ << " phase_3" << std::endl;

    if (values == Zero || values == One) {
        inc_round(values / 2);
        if (!decided_ && values / 2 == b) {
            set_decision();
        }
    } else {
        inc_round(b);
    }

    DVLOG(5) << net_.get_id() << " " << msg_base_ << " finished round: " << round_ - 1
             << " est: " << est_ << " bin_values: " << rounds_[round_ - 1].bin_values
             << " values: " << values;

    if (decided_ && round_ >= res_metrics_.rounds_number + 2) {
        state_ = Consensus;
        return;
    }

    phase_1();
}


void BinConsensus::continue_if_ready() {
    auto& round = rounds_[round_];
    if (state_ == BvBroadcast && round.bin_values != None) {
        if (!is_psync_) {
            phase_2();
            return;
        }

        phase_coord();
        if (round.timer_expired) {
            phase_2();
        }
    } else if (state_ == Broadcast) {
        if (round.received_AUXes.size() < nodes_cnt_ - (nodes_cnt_ - 1) / 3) {
            return;
        }

        if (round.bin_values != Both) {
            if (round.values[round.bin_values] < nodes_cnt_ - (nodes_cnt_ - 1) / 3) {
                return;
            }

            phase_3(static_cast<BinValues>(round.bin_values));
            return;
        }

        uint32_t values = 0;
        for (uint32_t i = BinValues::Zero; i <= BinValues::Both; ++i) {
            if (round.values[i] > 0) {
                values |= i;
            }
        }
        phase_3(static_cast<BinValues>(values));
    }
}

void BinConsensus::set_decision() {
    // time to-do
    assert(!decided_);

    decided_ = true;
    res_metrics_.decision = est_;
    res_metrics_.rounds_number = round_;

    DVLOG(5) << net_.get_id() << " " << msg_base_ << " DECIDED round: " 
             << res_metrics_.rounds_number << " decision: " << get_decision() << std::endl;
}

BinConsensusMetrics BinConsensus::get_metrics() {
    assert(state_ == Consensus);
    return res_metrics_;
}

void BinConsensus::execute_byzantine(Role role) {
    state_ = Consensus;
    if (role == FailStop) {
        return;
    }

    json EST_data(msg_base_), AUX_data(msg_base_), COORD_data(msg_base_);

    for (uint32_t round = 0; round < 10; ++round) {
        EST_data["round"] = round;
        for (uint32_t value = 0; value < (role == TxRejector ? 1 : 2); ++value) {
            EST_data["value"] = value;
            BV_.broadcast(EST_data);
        }

        if (is_psync_ && net_.get_id() == round % nodes_cnt_) {
            COORD_data["round"] = round;
            for (uint32_t value = 0; value < (role == TxRejector ? 1 : 2); ++value) {
                COORD_data["binvalues"] = value;
                net_.broadcast(Message("BinConsensus_COORD", COORD_data));
            }
        }

        AUX_data["round"] = round;
        AUX_data["binvalues"] = (role == TxRejector ? Zero : Both);
        net_.broadcast(Message("BinConsensus_AUX", AUX_data));
    }
}


BVbroadcast::BVbroadcast(int nodes_cnt, INetManager& net) : nodes_cnt_(nodes_cnt), net_(net) {
}

void BVbroadcast::broadcast(const json& data) {
    if (!data.contains("round") || !data.contains("value")) {
        return;
    }

    uint32_t round = data["round"];
    uint32_t value = data["value"];

    Instance& instance = get_instance(round);
    if ((value & 1) != value || instance.states_[value] >= Broadcast) {
        return;
    }

    net_.broadcast(Message("BV", data));
    instance.states_[value] = Broadcast;
}

std::optional<json> BVbroadcast::process_msg(Message msg) {
    DVLOG(6) << net_.get_id() << " bv-process" << msg << std::endl;
    if (!check_type(msg)) {
        return std::nullopt;
    }

    uint32_t value = msg.data["value"];
    DVLOG(6) << net_.get_id() << " " << msg << " value: " << value<< std::endl;
    if ((value & 1) != value) {
        return std::nullopt;
    }

    uint32_t round = msg.data["round"];
    DVLOG(6) << net_.get_id() << " " << msg << " round: " << round << std::endl;
    Instance& instance = get_instance(round);
    if (instance.states_[value] == Delivered) {
        return std::nullopt;
    }

    instance.received_from_[value].insert(msg.from);
    json a = {{"received_from", instance.received_from_[value]}};
    DVLOG(6) << net_.get_id() << " " << msg << " " << a << std::endl; 
    if (instance.received_from_[value].size() >= (nodes_cnt_ - 1) / 3 + 1
        && instance.states_[value] < Broadcast) {
            net_.broadcast(msg);
            instance.states_[value] = Broadcast;
    }

    if (instance.received_from_[value].size() >= (nodes_cnt_ - 1) / 3 * 2 + 1) {
        instance.states_[value] = Delivered;
        return msg.data;
    }

    return std::nullopt;
}

bool BVbroadcast::check_type(const Message& msg) const {
    if (msg.type != "BV") {
        return false;
    }

    return msg.data.contains("round") && msg.data.contains("value");
}

BVbroadcast::Instance& BVbroadcast::get_instance(uint32_t r) {
    if (instances_.size() <= r) {
        instances_.resize(r + 1);
    }

    return instances_[r];
}