#ifndef PYVRP_DYNAMICBITSET_H
#define PYVRP_DYNAMICBITSET_H

#include <bitset>
#include <vector>

namespace pyvrp
{
/**
 * DynamicBitset(num_bits: int)
 *
 * A simple dynamic bitset implementation. This class functions as a fast set
 * for membership checks on the integers. That is particularly useful for
 * testing if e.g. clients are in a solution or not.
 *
 * Parameters
 * ----------
 * num_bits
 *     Number of integers in ``[0, num_bits)`` this bitset must be able to
 *     store. If ``num_bits`` is not a multiple of ``BLOCK_SIZE``, the actual
 *     size is rounded up towards the next multiple.
 */
// See https://en.cppreference.com/w/cpp/utility/bitset for further information.
class DynamicBitset
{
    static constexpr size_t BLOCK_SIZE = 64;
    using Block = std::bitset<BLOCK_SIZE>;

    std::vector<Block> data_;

public:
    DynamicBitset(size_t numBits);
    DynamicBitset(std::vector<Block> data);

    [[nodiscard]] bool operator==(DynamicBitset const &other) const;

    [[nodiscard]] inline bool operator[](size_t idx) const;
    [[nodiscard]] inline Block::reference operator[](size_t idx);

    [[nodiscard]] size_t count() const;
    [[nodiscard]] size_t size() const;

    DynamicBitset &operator&=(DynamicBitset const &other);
    DynamicBitset &operator|=(DynamicBitset const &other);
    DynamicBitset &operator^=(DynamicBitset const &other);

    [[nodiscard]] DynamicBitset operator|(DynamicBitset const &other) const;
    [[nodiscard]] DynamicBitset operator&(DynamicBitset const &other) const;
    [[nodiscard]] DynamicBitset operator^(DynamicBitset const &other) const;
    [[nodiscard]] DynamicBitset operator~() const;
};

bool DynamicBitset::operator[](size_t idx) const
{
    auto const q = idx / BLOCK_SIZE;
    auto const r = idx % BLOCK_SIZE;
    return data_[q][r];
}

DynamicBitset::Block::reference DynamicBitset::operator[](size_t idx)
{
    auto const q = idx / BLOCK_SIZE;
    auto const r = idx % BLOCK_SIZE;
    return data_[q][r];
}
}  // namespace pyvrp

#endif  // PYVRP_DYNAMICBITSET_H
