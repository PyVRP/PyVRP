#include "DynamicBitset.h"

DynamicBitset::DynamicBitset(size_t numBits)
{
    auto const q = numBits / BLOCK_SIZE;
    auto const r = numBits % BLOCK_SIZE;
    data_ = {q + r > 0, 0};  // + 1 if the division is not perfect
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

size_t DynamicBitset::size() const { return 64 * data_.size(); }

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
    for (size_t idx = 0; idx != copy.size(); ++idx)
        copy[idx] = ~copy[idx];
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
