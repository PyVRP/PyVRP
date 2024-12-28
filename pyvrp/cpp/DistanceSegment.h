#ifndef PYVRP_DISTANCESEGMENT_H
#define PYVRP_DISTANCESEGMENT_H

#include "Matrix.h"
#include "Measure.h"

namespace pyvrp
{
/**
 * DistanceSegment(distance: int)
 *
 * Creates a distance segment.
 *
 * Distance segments can be efficiently concatenated, and track statistics
 * about route distance from visiting clients in the concatenated order.
 *
 * Parameters
 * ----------
 * distance
 *     Total distance in the route segment.
 */
class DistanceSegment
{
    Distance distance_ = 0;  // total distance

public:
    [[nodiscard]] static inline DistanceSegment
    merge(Distance const edgeDistance,
          DistanceSegment const &first,
          DistanceSegment const &second);

    /**
     * The total distance of this route segment.
     */
    [[nodiscard]] inline Distance distance() const;

    DistanceSegment() = default;  // default is all zero

    // Construct from raw data.
    inline DistanceSegment(Distance distance);

    // Move or copy construct from the other distance segment.
    inline DistanceSegment(DistanceSegment const &) = default;
    inline DistanceSegment(DistanceSegment &&) = default;

    // Move or copy assign from the other distance segment.
    inline DistanceSegment &operator=(DistanceSegment const &) = default;
    inline DistanceSegment &operator=(DistanceSegment &&) = default;
};

DistanceSegment DistanceSegment::merge(Distance const edgeDistance,
                                       DistanceSegment const &first,
                                       DistanceSegment const &second)
{
    return {first.distance_ + second.distance_ + edgeDistance};
}

Distance DistanceSegment::distance() const { return distance_; }

DistanceSegment::DistanceSegment(Distance distance) : distance_(distance) {}
}  // namespace pyvrp

#endif  // PYVRP_DISTANCESEGMENT_H
