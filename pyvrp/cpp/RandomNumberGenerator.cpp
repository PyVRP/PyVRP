#include "RandomNumberGenerator.h"

using pyvrp::RandomNumberGenerator;

RandomNumberGenerator::RandomNumberGenerator(uint32_t seed)
    : state_{seed, 123456789, 362436069, 521288629}
{
}

RandomNumberGenerator::RandomNumberGenerator(std::array<uint32_t, 4> state)
    : state_(std::move(state))
{
}

RandomNumberGenerator::result_type RandomNumberGenerator::operator()()
{
    // Algorithm "xor128" from p. 5 of Marsaglia, "Xorshift RNGs"
    uint32_t t = state_[3];

    // Perform a contrived 32-bit shift.
    uint32_t s = state_[0];
    state_[3] = state_[2];
    state_[2] = state_[1];
    state_[1] = s;

    t ^= t << 11;
    t ^= t >> 8;

    // Return the new random number
    return state_[0] = t ^ s ^ (s >> 19);
}

std::array<uint32_t, 4> const &RandomNumberGenerator::state() const
{
    return state_;
}
