#ifndef XORSHIFT128_H
#define XORSHIFT128_H

#include <climits>
#include <iosfwd>
#include <type_traits>

// This is a Xorshift random number generators, also called shift-register
// generators, which is a pseudorandom number generators. It generates the next
// number in the sequence by repeatedly taking the 'exclusive or' (the ^
// operator) of a number with a bit-shifted version of itself. For more
// information, see: https://en.wikipedia.org/wiki/Xorshift
class XorShift128
{
    // This random number generator uses 4 numbers.
    // Those numbers are used to repeatedly take the 'exclusive or' of a number
    // with a bit-shifted version of itself.
    unsigned state_[4]{};

public:
    typedef unsigned result_type;

    // Constructor of the Xorshift random number generator, given a seed stored
    // as state_[0]
    explicit XorShift128(const int seed = 42)
    {
        state_[0] = seed;
        state_[1] = 123456789;
        state_[2] = 362436069;
        state_[3] = 521288629;
    }

    // Return the min unsigned integer value
    static constexpr size_t min() { return 0; }

    // Return the max unsigned integer value
    static constexpr size_t max() { return UINT_MAX; }

    // A new random number will be returned when rng() is called on the
    // XorShift128 instance.
    result_type operator()()
    {
        // Algorithm "xor128" from p. 5 of Marsaglia, "Xorshift RNGs"
        unsigned t = state_[3];

        // Perform a contrived 32-bit shift.
        unsigned s = state_[0];
        state_[3] = state_[2];
        state_[2] = state_[1];
        state_[1] = s;

        t ^= t << 11;
        t ^= t >> 8;

        // Return the new random number
        return state_[0] = t ^ s ^ (s >> 19);
    }

    /**
     * Returns a random integer in the range [0, high).
     */
    template <typename T> result_type randint(T high)
    {
        static_assert(std::is_integral<T>::value);
        return operator()() % high;
    }
};

#endif
