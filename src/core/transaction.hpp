#pragma once

#include <cassert>
#include <deque>

using Transaction = uint64_t;

Transaction gen_tx();

class TransactionPool {
public:
    Transaction get_tx() {
        assert(!pool_.empty());
        Transaction tx = std::move(pool_.front());
        pool_.pop_front();
        return tx;
    }

    void add_tx(Transaction tx) {
        pool_.push_back(std::move(tx));
    }

    // in case tx wasn't decided in consensus algorithm
    void return_tx(Transaction tx) {
        pool_.push_front(std::move(tx));
    }

private:
    std::deque<Transaction> pool_;
};