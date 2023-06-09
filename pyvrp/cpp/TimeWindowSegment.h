#ifndef TIMEWINDOWDATA_H
#define TIMEWINDOWDATA_H

#include "Matrix.h"

class TimeWindowSegment
{
    using TWS = TimeWindowSegment;

    int idxFirst = 0;  // Index of the first client in the segment
    int idxLast = 0;   // Index of the last client in the segment
    int duration = 0;  // Total duration, incl. waiting and servicing
    int timeWarp = 0;  // Cumulative time warp
    int twEarly = 0;   // Earliest visit moment of first client
    int twLate = 0;    // Latest visit moment of last client

    [[nodiscard]] inline TWS merge(Matrix<int> const &durationMatrix,
                                   TWS const &other) const;

public:
    template <typename... Args>
    [[nodiscard]] inline static TWS merge(Matrix<int> const &durationMatrix,
                                          TWS const &first,
                                          TWS const &second,
                                          Args... args);

    /**
     * Total time warp, that is, the time warp along the the segment, and
     * potential time warp due to too late a release time.
     */
    [[nodiscard]] inline int totalTimeWarp() const;

    TimeWindowSegment() = default;

    inline TimeWindowSegment(int idxFirst,
                             int idxLast,
                             int duration,
                             int timeWarp,
                             int twEarly,
                             int twLate);
};

TimeWindowSegment TimeWindowSegment::merge(Matrix<int> const &durationMatrix,
                                           TimeWindowSegment const &other) const
{
#ifdef VRP_NO_TIME_WINDOWS
    return {};
#else
    auto const arcDuration = durationMatrix(idxLast, other.idxFirst);
    auto const delta = duration - timeWarp + arcDuration;
    auto const deltaWaitTime = std::max(other.twEarly - delta - twLate, 0);
    auto const deltaTimeWarp = std::max(twEarly + delta - other.twLate, 0);

    return {idxFirst,
            other.idxLast,
            duration + other.duration + arcDuration + deltaWaitTime,
            timeWarp + other.timeWarp + deltaTimeWarp,
            std::max(other.twEarly - delta, twEarly) - deltaWaitTime,
            std::min(other.twLate - delta, twLate) + deltaTimeWarp};
#endif
}

template <typename... Args>
TimeWindowSegment TimeWindowSegment::merge(Matrix<int> const &durationMatrix,
                                           TimeWindowSegment const &first,
                                           TimeWindowSegment const &second,
                                           Args... args)
{
#ifdef VRP_NO_TIME_WINDOWS
    return {};
#else
    auto const res = first.merge(durationMatrix, second);

    if constexpr (sizeof...(args) == 0)
        return res;
    else
        return merge(durationMatrix, res, args...);
#endif
}

int TimeWindowSegment::totalTimeWarp() const { return timeWarp; }

TimeWindowSegment::TimeWindowSegment(int idxFirst,
                                     int idxLast,
                                     int duration,
                                     int timeWarp,
                                     int twEarly,
                                     int twLate)
    : idxFirst(idxFirst),
      idxLast(idxLast),
      duration(duration),
      timeWarp(timeWarp),
      twEarly(twEarly),
      twLate(twLate)
{
}

#endif  // TIMEWINDOWDATA_H
