#ifndef PYVRP_DYNAMICBITSET_H
#define PYVRP_DYNAMICBITSET_H

#include <bitset>
#include <cassert>
#include <vector>

/**
 * A simple dynamic bitset implementation, on top of a vector of bitsets.
 */
class DynamicBitset
{
    static constexpr size_t BLOCK_SIZE = 64;
    using Block = std::bitset<BLOCK_SIZE>;

    std::vector<Block> data_;

public:
    DynamicBitset(size_t numBits)
    {
        auto const q = numBits / BLOCK_SIZE;
        auto const r = numBits % BLOCK_SIZE;
        data_ = {q + r > 0, 0};
    };

    DynamicBitset(std::vector<Block> data) : data_(std::move(data)){};

    [[nodiscard]] inline bool operator==(DynamicBitset const &other) const
    {
        return data_ == other.data_;
    }

    [[nodiscard]] inline bool operator[](size_t idx) const
    {
        auto const q = idx / BLOCK_SIZE;
        auto const r = idx % BLOCK_SIZE;
        return data_[q][r];
    }

    [[nodiscard]] inline Block::reference operator[](size_t idx)
    {
        auto const q = idx / BLOCK_SIZE;
        auto const r = idx % BLOCK_SIZE;
        return data_[q][r];
    }

    [[nodiscard]] inline size_t count() const
    {
        size_t count = 0;
        for (auto const &bitset : data_)
            count += bitset.count();
        return count;
    }

    [[nodiscard]] inline size_t size() const { return 64 * data_.size(); }

    DynamicBitset &operator&=(DynamicBitset const &other)
    {
        assert(size() == other.size());  // assumed true during runtime

        for (size_t idx = 0; idx != data_.size(); ++idx)
            data_[idx] &= other.data_[idx];

        return *this;
    }

    DynamicBitset &operator|=(DynamicBitset const &other)
    {
        assert(size() == other.size());  // assumed true during runtime

        for (size_t idx = 0; idx != data_.size(); ++idx)
            data_[idx] |= other.data_[idx];

        return *this;
    }

    DynamicBitset &operator^=(DynamicBitset const &other)
    {
        assert(size() == other.size());  // assumed true during runtime

        for (size_t idx = 0; idx != data_.size(); ++idx)
            data_[idx] ^= other.data_[idx];

        return *this;
    }

    DynamicBitset operator~() const
    {
        std::vector<Block> copy(data_);
        for (size_t idx = 0; idx != copy.size(); ++idx)
            copy[idx] = ~copy[idx];
        return copy;
    }

    [[nodiscard]] DynamicBitset operator|(DynamicBitset const &other) const
    {
        std::vector<Block> copy(data_);
        return DynamicBitset(copy) |= other;
    }

    [[nodiscard]] DynamicBitset operator&(DynamicBitset const &other) const
    {
        std::vector<Block> copy(data_);
        return DynamicBitset(copy) &= other;
    }

    [[nodiscard]] DynamicBitset operator^(DynamicBitset const &other) const
    {
        std::vector<Block> copy(data_);
        return DynamicBitset(copy) ^= other;
    }
};

#endif  // PYVRP_DYNAMICBITSET_H
