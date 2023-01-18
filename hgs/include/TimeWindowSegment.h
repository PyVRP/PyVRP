#ifndef TIMEWINDOWDATA_H
#define TIMEWINDOWDATA_H

#include "Matrix.h"

class TimeWindowSegment
{
    using TWS = TimeWindowSegment;

    Matrix<int> const *dist = nullptr;  // Distance matrix
    int idxFirst = 0;  // Index of the first client in the segment
    int idxLast = 0;   // Index of the last client in the segment
    int duration = 0;  // Total duration, incl. waiting and servicing
    int timeWarp = 0;  // Cumulative time warp
    int twEarly = 0;   // Earliest visit moment of first client
    int twLate = 0;    // Latest visit moment of last client
    int release = 0;   // Release time; cannot leave depot earlier

    [[nodiscard]] TWS merge(TWS const &other) const;

public:
    template <typename... Args>
    [[nodiscard]] static TWS
    merge(TWS const &first, TWS const &second, Args... args);

    /**
     * Returns the time warp along the segment, assuming we can depart in time.
     */
    [[nodiscard]] int segmentTimeWarp() const;

    /**
     * Total time warp, that is, the time warp along the the segment, and
     * potential time warp due to too late a release time.
     */
    [[nodiscard]] int totalTimeWarp() const;

    TimeWindowSegment() = default;  // TODO get rid of this constructor

    TimeWindowSegment(Matrix<int> const *dist,
                      int idxFirst,
                      int idxLast,
                      int duration,
                      int timeWarp,
                      int twEarly,
                      int twLate,
                      int release);
};

template <typename... Args>
TimeWindowSegment TimeWindowSegment::merge(TimeWindowSegment const &first,
                                           TimeWindowSegment const &second,
                                           Args... args)
{
    auto const res = first.merge(second);

    if constexpr (sizeof...(args) == 0)
        return res;
    else
        return merge(res, args...);
}

#endif  // TIMEWINDOWDATA_H
