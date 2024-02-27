#ifndef PYVRP_LOADSEGMENT_H
#define PYVRP_LOADSEGMENT_H

#include "Measure.h"
#include "ProblemData.h"

namespace pyvrp
{
/**
 * LoadSegment(delivery: int = 0, pickup: int = 0, load: int = 0)
 *
 * Creates a new load segment. Load segments can be efficiently concatenated,
 * and track statistics about capacity violations resulting from visiting
 * clients in the concatenated order.
 *
 * Parameters
 * ----------
 * delivery
 *     Total delivery amount on this segment.
 * pickup
 *     Total pickup amount on this segment.
 * load
 *     Maximum load on this segment.
 */
class LoadSegment
{
    Load delivery_;
    Load pickup_;
    Load load_;

public:
    template <typename... Args>
    [[nodiscard]] static LoadSegment
    merge(LoadSegment const &first, LoadSegment const &second, Args &&...args);

    /**
     * Returns the delivery amount, that is, the total amount of load delivered
     * to clients on this segment.
     */
    [[nodiscard]] Load delivery() const;

    /**
     * Returns the amount picked up from clients on this segment.
     */
    [[nodiscard]] Load pickup() const;

    /**
     * Returns the maximum load encountered on this segment.
     */
    [[nodiscard]] inline Load load() const;

    // Construct from attributes of the given client.
    LoadSegment(ProblemData::Client const &client);

    // Construct from raw data.
    inline LoadSegment(Load delivery, Load pickup, Load load);

    // Move or copy construct from the other load segment.
    inline LoadSegment(LoadSegment const &) = default;
    inline LoadSegment(LoadSegment &&) = default;

    // Move or copy assign form the other load segment.
    inline LoadSegment &operator=(LoadSegment const &) = default;
    inline LoadSegment &operator=(LoadSegment &&) = default;
};

template <typename... Args>
LoadSegment LoadSegment::merge(LoadSegment const &first,
                               LoadSegment const &second,
                               Args &&...args)
{
    // See Vidal et al. (2014) for details. This function implements equations
    // (9) -- (11) of https://doi.org/10.1016/j.ejor.2013.09.045.
    LoadSegment const res = {
        first.delivery_ + second.delivery_,
        first.pickup_ + second.pickup_,
        std::max(first.load_ + second.delivery_, second.load_ + first.pickup_)};

    if constexpr (sizeof...(args) == 0)
        return res;
    else
        return merge(res, args...);
}

Load LoadSegment::load() const { return load_; }

LoadSegment::LoadSegment(Load delivery, Load pickup, Load load)
    : delivery_(delivery), pickup_(pickup), load_(load)
{
}
}  // namespace pyvrp

#endif  // PYVRP_LOADSEGMENT_H
