#ifndef PYVRP_LOADSEGMENT_H
#define PYVRP_LOADSEGMENT_H

#include "Activity.h"
#include "Measure.h"
#include "ProblemData.h"

#include <iosfwd>

namespace pyvrp
{
/**
 * LoadSegment(initial: int, delta: int, increase: int, excess_load: int = 0)
 *
 * Creates a new load segment for delivery and pickup loads in a single
 * dimension. These load segments can be efficiently concatenated, and track
 * statistics about capacity violations resulting from visiting clients and
 * shipments in the concatenated order.
 *
 * Parameters
 * ----------
 * initial
 *     Initial load on the segment, loaded at the last depot visit.
 * delta
 *     Load delta since last depot visit.
 * increase
 *     Maximum load increase since last depot visit.
 * excess_load
 *     Cumulative excess load on this segment, possibly from earlier trips.
 */
class LoadSegment
{
    Load initial_ = 0;     // loaded at last depot visit
    Load delta_ = 0;       // delta since last depot visit
    Load increase_ = 0;    // additional load added during the trip
    Load excessLoad_ = 0;  // cumulative excess load over other trips in segment

public:
    [[nodiscard]] static inline LoadSegment merge(LoadSegment const &first,
                                                  LoadSegment const &second);

    /**
     * Finalises the load on this segment, and returns a new segment where any
     * excess load has been moved to the cumulative excess load field. This is
     * useful with reloading, because the finalised segment can be concatenated
     * with load segments of subsequent trips.
     */
    [[nodiscard]] inline LoadSegment finalise(Load capacity) const;

    /**
     * Initial load on this segment. This is the amount loaded at the last
     * depot visit.
     */
    [[nodiscard]] Load initial() const;

    /**
     * This is the amount by which load has maximally increased since the last
     * depot visit.
     */
    [[nodiscard]] Load increase() const;

    /**
     * Actual load delta since the last depot visit.
     */
    [[nodiscard]] Load delta() const;

    /**
     * Returns the maximum load encountered since the last depot visit.
     */
    [[nodiscard]] inline Load load() const;

    /**
     * Returns the load violation on this segment.
     *
     * Parameters
     * ----------
     * capacity
     *     Vehicle capacity.
     */
    [[nodiscard]] inline Load excessLoad(Load capacity) const;

    LoadSegment() = default;  // default is all zero

    // Construct from load attributes of the given client and dimension.
    LoadSegment(Client const &client, size_t dimension);

    // Construct from load attributes of the given shipment and dimension.
    LoadSegment(Shipment const &shipment,
                Activity::ActivityType type,
                size_t dimension);

    // Construct from initial load attributes of the given vehicle type and
    // dimension.
    LoadSegment(VehicleType const &vehicleType, size_t dimension);

    // Construct from raw data.
    inline LoadSegment(Load initial,
                       Load delta,
                       Load increase,
                       Load excessLoad = 0);

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
    return {first.initial_ + second.initial_,
            first.delta_ + second.delta_,
            std::max(first.increase_, first.delta_ + second.increase_),
            first.excessLoad_ + second.excessLoad_};
}

Load LoadSegment::load() const { return initial_ + increase_; }

Load LoadSegment::excessLoad(Load capacity) const
{
    return excessLoad_ + std::max<Load>(load() - capacity, 0);
}

LoadSegment LoadSegment::finalise(Load capacity) const
{
    return {0, 0, 0, excessLoad(capacity)};
}

LoadSegment::LoadSegment(Load initial,
                         Load delta,
                         Load increase,
                         Load excessLoad)
    : initial_(initial),
      delta_(delta),
      increase_(increase),
      excessLoad_(excessLoad)
{
}
}  // namespace pyvrp

std::ostream &operator<<(std::ostream &out,  // helpful for debugging
                         pyvrp::LoadSegment const &segment);

#endif  // PYVRP_LOADSEGMENT_H
