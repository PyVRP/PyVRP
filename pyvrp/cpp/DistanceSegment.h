#ifndef PYVRP_DISTANCESEGMENT_H
#define PYVRP_DISTANCESEGMENT_H

#include "Matrix.h"
#include "Measure.h"

namespace pyvrp
{
/**
 * DistanceSegment(
 *     idx_first: int,
 *     idx_last: int,
 *     distance: int,
 * )
 *
 * Creates a distance segment.
 *
 * Distance segments can be efficiently concatenated, and track statistics
 * about route distance from visiting clients in the concatenated order.
 *
 * Parameters
 * ----------
 * idx_first
 *     Index of the first client in the route segment.
 * idx_last
 *     Index of the last client in the route segment.
 * distance
 *     Total distance in the route segment.
 */
class DistanceSegment
{
    size_t idxFirst_;    // Index of the first client in the segment
    size_t idxLast_;     // Index of the last client in the segment
    Distance distance_;  // Total distance

public:
    template <typename... Args>
    [[nodiscard]] static DistanceSegment
    merge(Matrix<Distance> const &distanceMatrix,
          DistanceSegment const &first,
          DistanceSegment const &second,
          Args &&...args);

    /**
     * The total distance of this route segment.
     */
    [[nodiscard]] inline Distance distance() const;

    // Construct an empty distance segment for the given client index.
    DistanceSegment(size_t idx);

    // Construct from raw data.
    inline DistanceSegment(size_t idxFirst, size_t idxLast, Distance distance);

    // Move or copy construct from the other distance segment.
    inline DistanceSegment(DistanceSegment const &) = default;
    inline DistanceSegment(DistanceSegment &&) = default;

    // Move or copy assign from the other distance segment.
    inline DistanceSegment &operator=(DistanceSegment const &) = default;
    inline DistanceSegment &operator=(DistanceSegment &&) = default;
};

template <typename... Args>
DistanceSegment DistanceSegment::merge(Matrix<Distance> const &distanceMatrix,
                                       DistanceSegment const &first,
                                       DistanceSegment const &second,
                                       Args &&...args)
{
    auto const edgeDistance = distanceMatrix(first.idxLast_, second.idxFirst_);
    DistanceSegment const res
        = {first.idxFirst_,
           second.idxLast_,
           first.distance_ + second.distance_ + edgeDistance};

    if constexpr (sizeof...(args) == 0)
        return res;
    else
        return merge(distanceMatrix, res, args...);
}

Distance DistanceSegment::distance() const { return distance_; }

DistanceSegment::DistanceSegment(size_t idxFirst,
                                 size_t idxLast,
                                 Distance distance)
    : idxFirst_(idxFirst), idxLast_(idxLast), distance_(distance)
{
}
}  // namespace pyvrp

#endif  // PYVRP_DISTANCESEGMENT_H
