#ifndef PYVRP_DYNAMICBITSET_H
#define PYVRP_DYNAMICBITSET_H

#include "roaring.hh"

/**
 * A thin wrapper around CRoaring's ``Roaring`` bitset.
 */
class DynamicBitset
{
    roaring::Roaring bitset;

public:
    DynamicBitset() = default;

    DynamicBitset(roaring::Roaring bitset) : bitset(std::move(bitset)){};

    DynamicBitset(size_t n, uint32_t const *data) : bitset(n, data){};

    inline void insert(uint32_t val);

    [[nodiscard]] inline bool contains(uint32_t val);

    inline void remove(uint32_t val);

    [[nodiscard]] inline bool empty() const;

    [[nodiscard]] inline size_t size() const;

    [[nodiscard]] inline bool operator==(DynamicBitset const &other) const;

    [[nodiscard]] DynamicBitset operator-(DynamicBitset const &other) const;
    DynamicBitset &operator-=(DynamicBitset const &other);

    [[nodiscard]] DynamicBitset operator|(DynamicBitset const &other) const;
    DynamicBitset &operator|=(DynamicBitset const &other);

    [[nodiscard]] DynamicBitset operator&(DynamicBitset const &other) const;
    DynamicBitset &operator&=(DynamicBitset const &other);

    [[nodiscard]] DynamicBitset operator^(DynamicBitset const &other) const;
    DynamicBitset &operator^=(DynamicBitset const &other);
};

void DynamicBitset::insert(uint32_t val) { bitset.add(val); }

bool DynamicBitset::contains(uint32_t val) { return bitset.contains(val); }

void DynamicBitset::remove(uint32_t val) { return bitset.remove(val); }

bool DynamicBitset::empty() const { return bitset.isEmpty(); }

size_t DynamicBitset::size() const { return bitset.cardinality(); }

bool DynamicBitset::operator==(DynamicBitset const &other) const
{
    return bitset == other.bitset;
}

DynamicBitset DynamicBitset::operator-(DynamicBitset const &other) const
{
    return bitset - other.bitset;
}

DynamicBitset &DynamicBitset::operator-=(DynamicBitset const &other)
{
    bitset -= other.bitset;
    return *this;
}

DynamicBitset DynamicBitset::operator|(DynamicBitset const &other) const
{
    return bitset | other.bitset;
}

DynamicBitset &DynamicBitset::operator|=(DynamicBitset const &other)
{
    bitset |= other.bitset;
    return *this;
}

DynamicBitset DynamicBitset::operator&(DynamicBitset const &other) const
{
    return bitset & other.bitset;
}

DynamicBitset &DynamicBitset::operator&=(DynamicBitset const &other)
{
    bitset &= other.bitset;
    return *this;
}

DynamicBitset DynamicBitset::operator^(DynamicBitset const &other) const
{
    return bitset ^ other.bitset;
}

DynamicBitset &DynamicBitset::operator^=(DynamicBitset const &other)
{
    bitset ^= other.bitset;
    return bitset;
}

#endif  // PYVRP_DYNAMICBITSET_H
