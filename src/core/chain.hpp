#pragma once

#include <unordered_set>

#include "transaction.hpp"

struct Block {
    uint32_t block_id;
    std::vector<Transaction> data;

    size_t size() {
        return data.size();
    }

    // imitation of identification of conflicts, just to test algorithm
    bool conflicts(Transaction& other_tx) {
        static const int MOD = 73;

        for (auto& tx : data) {
            if (tx == other_tx || tx % MOD == other_tx % MOD) {
                return true;
            }
        }

        return false;
    }

    friend std::ostream& operator<<(std::ostream& out, Block block) {
        out << "{";
        for (auto& tx : block.data) {
            out << tx << " ";
        }
        out << "}";

        return out;
    }
};

class Chain {
public:
    void add_block(Block& block) {
        blocks_.push_back(block);
        for (auto& tx: block.data) {
            all_tx_.insert(tx);
        }
    }

    size_t get_height() {
        return blocks_.size();
    }

    // imitation of identification of conflicts, just to test algorithm
    bool conflicts(Transaction& tx) {
        return all_tx_.contains(tx);
    }

    Block& get_block(size_t id) {
        return blocks_[id];
    }

private:
    std::unordered_set<Transaction> all_tx_;
    std::vector<Block> blocks_;
};