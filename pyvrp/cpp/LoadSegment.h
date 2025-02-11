#ifndef PYVRP_LOADSEGMENT_H
#define PYVRP_LOADSEGMENT_H

#include "Measure.h"
#include "ProblemData.h"

namespace pyvrp
{
/**
 * LoadSegment(delivery: int = 0, pickup: int = 0, load: int = 0)
 *
 * Creates a new load segment for delivery and pickup loads in a single
 * dimension. These load segments can be efficiently concatenated, and track
 * statistics about capacity violations resulting from visiting clients in the
 * concatenated order.
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
    Load delivery_ = 0;
    Load pickup_ = 0;
    Load load_ = 0;

public:
    [[nodiscard]] static inline LoadSegment merge(LoadSegment const &first,
                                                  LoadSegment const &second);

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

    LoadSegment() = default;  // default is all zero

    // Construct from load attributes of the given client and dimension.
    LoadSegment(ProblemData::Client const &client, size_t dimension);

    // Construct from initial load attributes of the given vehicle type and
    // dimension.
    LoadSegment(ProblemData::VehicleType const &vehicleType, size_t dimension);

    // Construct from raw data.
    inline LoadSegment(Load delivery, Load pickup, Load load);

    // Move or copy construct from the other load segment.
    inline LoadSegment(LoadSegment const &) = default;
    inline LoadSegment(LoadSegment &&) = default;

    // Move or copy assign form the other load segment.
    inline LoadSegment &operator=(LoadSegment const &) = default;
    inline LoadSegment &operator=(LoadSegment &&) = default;
};

LoadSegment LoadSegment::merge(LoadSegment const &first,
                               LoadSegment const &second)
{
    // See Vidal et al. (2014) for details. This function implements equations
    // (9) -- (11) of https://doi.org/10.1016/j.ejor.2013.09.045.
    return {
        first.delivery_ + second.delivery_,
        first.pickup_ + second.pickup_,
        std::max(first.load_ + second.delivery_, second.load_ + first.pickup_)};
}

Load LoadSegment::load() const { return load_; }

LoadSegment::LoadSegment(Load delivery, Load pickup, Load load)
    : delivery_(delivery), pickup_(pickup), load_(load)
{
}
}  // namespace pyvrp

#endif  // PYVRP_LOADSEGMENT_H
