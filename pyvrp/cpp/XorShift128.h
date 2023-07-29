#ifndef PYVRP_XORSHIFT128_H
#define PYVRP_XORSHIFT128_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace pyvrp
{
/**
 * This class implements a XOR-shift pseudo-random number generator. It
 * generates the next number of a sequence by repeatedly taking the 'exclusive
 * or' (the ^ operator) of a number with a bit-shifted version of itself. See
 * also here for more details: https://en.wikipedia.org/wiki/Xorshift.
 */
class XorShift128
{
    uint32_t state_[4]{};

public:
    typedef uint32_t result_type;

    /**
     * Constructs a XOR-shift pseudo-RNG, seeded at the given seed.
     *
     * @param seed Used to seed the pseudo-RNG state.
     */
    explicit XorShift128(uint32_t seed);

    /**
     * @return The minimum value this pRNG can generate.
     */
    static constexpr size_t min();

    /**
     * @return The maximum value this pRNG can generate.
     */
    static constexpr size_t max();

    /**
     * Generates one pseudo-random integer in the range <code>[min(), max())
     * </code>.
     *
     * @return A pseudo-random integer.
     */
    result_type operator()();

    /**
     * Generates one pseudo-random double uniformly in the range [0, 1].
     *
     * @return A pseudo-random number in the range [0, 1].
     */
    template <typename T> T rand();

    /**
     * Generates one pseudo-random integer in the range <code>[0, high)</code>.
     *
     * @param high Upper bound on the integer to generate.
     * @return A pseudo-random integer.
     */
    template <typename T> result_type randint(T high);
};

constexpr size_t XorShift128::min()
{
    return std::numeric_limits<result_type>::min();
}

constexpr size_t XorShift128::max()
{
    return std::numeric_limits<result_type>::max();
}

template <typename T> T XorShift128::rand()
{
    static_assert(std::is_floating_point<T>::value);
    return operator()() / static_cast<T>(max());
}

template <typename T> XorShift128::result_type XorShift128::randint(T high)
{
    static_assert(std::is_integral<T>::value);
    return operator()() % high;
}
}  // namespace pyvrp

#endif  // PYVRP_XORSHIFT128_H
