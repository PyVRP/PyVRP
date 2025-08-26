#ifndef PYVRP_RANDOMNUMBERGENERATOR_H
#define PYVRP_RANDOMNUMBERGENERATOR_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace pyvrp
{
/**
 * RandomNumberGenerator(seed: int)
 *
 * This class implements a XOR-shift pseudo-random number generator (RNG). It
 * generates the next number of a sequence by repeatedly taking the 'exclusive
 * or' (the ``^`` operator) of a number with a bit-shifted version of itself.
 * See `here <https://en.wikipedia.org/wiki/Xorshift>`_ for more details.
 *
 * Parameters
 * ----------
 * seed
 *     Seed used to set the initial RNG state.
 */
class RandomNumberGenerator
{
    std::array<uint32_t, 4> state_;

public:
    typedef uint32_t result_type;

    explicit RandomNumberGenerator(uint32_t seed);
    explicit RandomNumberGenerator(std::array<uint32_t, 4> state);

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
    double rand();

    /**
     * Generates one pseudo-random integer in the range <code>[0, high)</code>.
     *
     * @param high Upper bound on the integer to generate.
     * @return A pseudo-random integer.
     */
    template <typename T> result_type randint(T high);

    /**
     * Randomly shuffles the elements in the given range in-place using the
     * Fisher-Yates algorithm.
     *
     * @param first Iterator to the beginning of the range to shuffle.
     * @param last Iterator to the end of the range to shuffle.
     */
    template <typename RandomIt> void shuffle(RandomIt first, RandomIt last);

    /**
     * Returns the internal RNG state.
     */
    // Could be useful for debugging.
    std::array<uint32_t, 4> const &state() const;
};

constexpr size_t RandomNumberGenerator::min()
{
    return std::numeric_limits<result_type>::min();
}

constexpr size_t RandomNumberGenerator::max()
{
    return std::numeric_limits<result_type>::max();
}

template <typename T>
RandomNumberGenerator::result_type RandomNumberGenerator::randint(T high)
{
    static_assert(std::is_integral<T>::value);
    return operator()() % high;
}

template <typename RandomIt>
void RandomNumberGenerator::shuffle(RandomIt first, RandomIt last)
{
    // Taken from https://en.cppreference.com/w/cpp/algorithm/random_shuffle.
    for (auto idx = last - first - 1; idx > 0; --idx)
        std::swap(first[idx], first[randint(idx + 1)]);
}
}  // namespace pyvrp

#endif  // PYVRP_RANDOMNUMBERGENERATOR_H
