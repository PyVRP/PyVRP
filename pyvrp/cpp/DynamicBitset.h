#ifndef PYVRP_DYNAMICBITSET_H
#define PYVRP_DYNAMICBITSET_H

#include <vector>

class DynamicBitset
{
    std::vector<bool> data_;

public:
    DynamicBitset() = default;

    DynamicBitset(size_t n, bool default = false);

    [[nodiscard]] decltype(auto) operator[](size_t idx);

    [[nodiscard]] decltype(auto) operator[](size_t idx) const;

    // TODO
}

#endif  // PYVRP_DYNAMICBITSET_H
