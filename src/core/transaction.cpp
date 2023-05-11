#include <random>
#include "transaction.hpp"

Transaction gen_tx() {
    static std::mt19937 gen = std::mt19937(std::random_device()());
    static auto rand_engine = std::uniform_int_distribution<>(1);
    return rand_engine(gen);
}