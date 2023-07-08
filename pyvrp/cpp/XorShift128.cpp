#include "XorShift128.h"

using pyvrp::XorShift128;

XorShift128::XorShift128(uint32_t seed)
{
    state_[0] = seed;
    state_[1] = 123456789;
    state_[2] = 362436069;
    state_[3] = 521288629;
}

XorShift128::result_type XorShift128::operator()()
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
