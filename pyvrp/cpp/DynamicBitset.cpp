#include "DynamicBitset.h"

#include <cassert>

using pyvrp::DynamicBitset;

DynamicBitset::DynamicBitset(size_t numBits)
    // numBits / BLOCK_SIZE blocks, with an adjustment in case the division of
    // numBits by BLOCK_SIZE is not perfect. Zero initialise each block.
    : data_(numBits / BLOCK_SIZE + (numBits % BLOCK_SIZE != 0), 0)
{
}

DynamicBitset::DynamicBitset(std::vector<Block> data) : data_(std::move(data))
{
}

bool DynamicBitset::operator==(DynamicBitset const &other) const
{
    return data_ == other.data_;
}

DynamicBitset &DynamicBitset::operator&=(DynamicBitset const &other)
{
    assert(size() == other.size());  // assumed true during runtime

    for (size_t idx = 0; idx != data_.size(); ++idx)
        data_[idx] &= other.data_[idx];

    return *this;
}

size_t DynamicBitset::count() const
{
    size_t count = 0;
    for (auto const &bitset : data_)
        count += bitset.count();
    return count;
}

size_t DynamicBitset::size() const { return BLOCK_SIZE * data_.size(); }

DynamicBitset &DynamicBitset::operator|=(DynamicBitset const &other)
{
    assert(size() == other.size());  // assumed true during runtime

    for (size_t idx = 0; idx != data_.size(); ++idx)
        data_[idx] |= other.data_[idx];

    return *this;
}

DynamicBitset &DynamicBitset::operator^=(DynamicBitset const &other)
{
    assert(size() == other.size());  // assumed true during runtime

    for (size_t idx = 0; idx != data_.size(); ++idx)
        data_[idx] ^= other.data_[idx];

    return *this;
}

DynamicBitset DynamicBitset::operator~() const
{
    std::vector<Block> copy(data_);
    for (size_t idx = 0; idx != data_.size(); ++idx)
        copy[idx] = ~data_[idx];
    return copy;
}

DynamicBitset DynamicBitset::operator|(DynamicBitset const &other) const
{
    std::vector<Block> copy(data_);
    return DynamicBitset(copy) |= other;
}

DynamicBitset DynamicBitset::operator&(DynamicBitset const &other) const
{
    std::vector<Block> copy(data_);
    return DynamicBitset(copy) &= other;
}

DynamicBitset DynamicBitset::operator^(DynamicBitset const &other) const
{
    std::vector<Block> copy(data_);
    return DynamicBitset(copy) ^= other;
}
