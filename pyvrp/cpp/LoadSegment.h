#ifndef PYVRP_LOADSEGMENT_H
#define PYVRP_LOADSEGMENT_H

#include "Measure.h"
#include "ProblemData.h"

namespace pyvrp
{
/**
 * LoadSegment(demand: int = 0, supply: int = 0, max_load: int = 0)
 *
 * Creates a new load segment. Load segments can be efficiently concatenated,
 * and track statistics about capacity violations resulting from visiting
 * clients in the concatenated order.
 *
 * Parameters
 * ----------
 * demand
 *     Total demand on this segment.
 * supply
 *     Total supply on this segment.
 * max_load
 *     Maximum load on this segment.
 */
class LoadSegment
{
    Load demand_;
    Load supply_;
    Load maxLoad_;

    [[nodiscard]] inline LoadSegment merge(LoadSegment const &other) const;

public:
    template <typename... Args>
    [[nodiscard]] inline static LoadSegment
    merge(LoadSegment const &first, LoadSegment const &second, Args &&...args);

    /**
     * Returns the excess load encountered on this segment, for a given
     * vehicle capacity.
     *
     * Parameters
     * ----------
     * capacity
     *     The given vehicle capacity.
     */
    [[nodiscard]] inline Load excessLoad(Load capacity) const;

    /**
     * Returns the demand, that is, the total amount of load delivered to
     * clients on this segment.
     */
    [[nodiscard]] Load demand() const;

    /**
     * Returns the supply, that is, the total amount of load picked up from
     * clients on this segment.
     */
    [[nodiscard]] Load supply() const;

    /**
     * Returns the maximum load encountered on this segment.
     */
    [[nodiscard]] Load maxLoad() const;

    // Construct from attributes of the given client.
    LoadSegment(ProblemData::Client const &client);

    // Construct from raw data.
    LoadSegment(Load demand, Load supply, Load maxLoad);
};

LoadSegment LoadSegment::merge(LoadSegment const &other) const
{
    // See Vidal et al. (2014) for details. This function implements equations
    // (9) -- (11) of https://doi.org/10.1016/j.ejor.2013.09.045.
    return {demand_ + other.demand_,
            supply_ + other.supply_,
            std::max(maxLoad_ + other.demand_, other.maxLoad_ + supply_)};
}

template <typename... Args>
LoadSegment LoadSegment::merge(LoadSegment const &first,
                               LoadSegment const &second,
                               Args &&...args)
{
    auto const res = first.merge(second);

    if constexpr (sizeof...(args) == 0)
        return res;
    else
        return merge(res, args...);
}

Load LoadSegment::excessLoad(Load capacity) const
{
    return std::max<Load>(maxLoad_ - capacity, 0);
}
}  // namespace pyvrp

#endif  // PYVRP_LOADSEGMENT_H
