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

    [[nodiscard]] inline TWS merge(TWS const &other) const;

public:
    template <typename... Args>
    [[nodiscard]] inline static TWS
    merge(TWS const &first, TWS const &second, Args... args);

    /**
     * Total time warp, that is, the time warp along the the segment, and
     * potential time warp due to too late a release time.
     */
    [[nodiscard]] inline int totalTimeWarp() const;

    TimeWindowSegment() = default;  // TODO get rid of this constructor

    inline TimeWindowSegment(Matrix<int> const *dist,
                             int idxFirst,
                             int idxLast,
                             int duration,
                             int timeWarp,
                             int twEarly,
                             int twLate);
};

template <typename... Args>
TimeWindowSegment TimeWindowSegment::merge(TimeWindowSegment const &first,
                                           TimeWindowSegment const &second,
                                           Args... args)
{
#ifdef VRP_NO_TIME_WINDOWS
    return {};
#else
    auto const res = first.merge(second);

    if constexpr (sizeof...(args) == 0)
        return res;
    else
        return merge(res, args...);
#endif
}

TimeWindowSegment TimeWindowSegment::merge(TimeWindowSegment const &other) const
{
#ifdef VRP_NO_TIME_WINDOWS
    return {};
#else
    int const distance = (*dist)(idxLast, other.idxFirst);
    int const delta = duration - timeWarp + distance;
    int const deltaWaitTime = std::max(other.twEarly - delta - twLate, 0);
    int const deltaTimeWarp = std::max(twEarly + delta - other.twLate, 0);

    return {dist,
            idxFirst,
            other.idxLast,
            duration + other.duration + distance + deltaWaitTime,
            timeWarp + other.timeWarp + deltaTimeWarp,
            std::max(other.twEarly - delta, twEarly) - deltaWaitTime,
            std::min(other.twLate - delta, twLate) + deltaTimeWarp};
#endif
}

int TimeWindowSegment::totalTimeWarp() const
{
    return timeWarp;
}

TimeWindowSegment::TimeWindowSegment(Matrix<int> const *dist,
                                     int idxFirst,
                                     int idxLast,
                                     int duration,
                                     int timeWarp,
                                     int twEarly,
                                     int twLate)
    : dist(dist),
      idxFirst(idxFirst),
      idxLast(idxLast),
      duration(duration),
      timeWarp(timeWarp),
      twEarly(twEarly),
      twLate(twLate)
{
}

#endif  // TIMEWINDOWDATA_H
