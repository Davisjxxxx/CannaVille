#pragma once

#include <cstdint>

namespace cannaville::core {

// Explicit, instance-owned RNG infrastructure. Simulation code must pass an
// instance or a derived stream; no global generator is permitted.
class DeterministicRng {
public:
    explicit DeterministicRng(std::uint64_t seed = 0) : state_(seed), seed_(seed) {}

    std::uint64_t next_u64() {
        state_ += 0x9E3779B97F4A7C15ULL;
        std::uint64_t z = state_;
        z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31U);
    }

    std::uint64_t seed() const { return seed_; }
    std::uint64_t state() const { return state_; }

    void restore_state(std::uint64_t seed, std::uint64_t state) {
        seed_ = seed;
        state_ = state;
    }

private:
    std::uint64_t state_;
    std::uint64_t seed_;
};

} // namespace cannaville::core
