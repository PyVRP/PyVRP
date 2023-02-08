#ifndef TIMEWINDOWDATA_H
#define TIMEWINDOWDATA_H

#include "Matrix.h"

#ifdef INT_PRECISION
using TCost = int;
using TDist = int;
using TTime = int;
#else
using TCost = double;
using TDist = double;
using TTime = double;
#endif

class TimeWindowSegment
{
    using TWS = TimeWindowSegment;

    Matrix<TTime> const *durationMatrix = nullptr;  // Duration matrix
    int idxFirst = 0;  // Index of the first client in the segment
    int idxLast = 0;   // Index of the last client in the segment
    TTime duration = 0;  // Total duration, incl. waiting and servicing
    TTime timeWarp = 0;  // Cumulative time warp
    TTime twEarly = 0;   // Earliest visit moment of first client
    TTime twLate = 0;    // Latest visit moment of last client
    TTime release = 0;   // Release time; cannot leave depot earlier

    [[nodiscard]] inline TWS merge(TWS const &other) const;

public:
    template <typename... Args>
    [[nodiscard]] inline static TWS
    merge(TWS const &first, TWS const &second, Args... args);

    /**
     * Returns the time warp along the segment, assuming we can depart in time.
     */
    [[nodiscard]] inline TTime segmentTimeWarp() const;

    /**
     * Total time warp, that is, the time warp along the the segment, and
     * potential time warp due to too late a release time.
     */
    [[nodiscard]] inline TTime totalTimeWarp() const;

    TimeWindowSegment() = default;  // TODO get rid of this constructor

    inline TimeWindowSegment(Matrix<TTime> const *durationMatrix,
                             int idxFirst,
                             int idxLast,
                             TTime duration,
                             TTime timeWarp,
                             TTime twEarly,
                             TTime twLate,
                             TTime release);
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

TimeWindowSegment TimeWindowSegment::merge(TimeWindowSegment const &other) const
{
    int const distance = (*durationMatrix)(idxLast, other.idxFirst);
    TTime const delta = duration - timeWarp + distance;
    TTime const deltaWaitTime = std::max(other.twEarly - delta - twLate, static_cast<TTime>(0));
    TTime const deltaTimeWarp = std::max(twEarly + delta - other.twLate, static_cast<TTime>(0));

    return {durationMatrix,
            idxFirst,
            other.idxLast,
            duration + other.duration + distance + deltaWaitTime,
            timeWarp + other.timeWarp + deltaTimeWarp,
            std::max(other.twEarly - delta, twEarly) - deltaWaitTime,
            std::min(other.twLate - delta, twLate) + deltaTimeWarp,
            std::max(release, other.release)};
}

TTime TimeWindowSegment::segmentTimeWarp() const { return timeWarp; }

TTime TimeWindowSegment::totalTimeWarp() const
{
    return segmentTimeWarp() + std::max(release - twLate, static_cast<TTime>(0));
}

TimeWindowSegment::TimeWindowSegment(Matrix<TTime> const *durationMatrix,
                                     int idxFirst,
                                     int idxLast,
                                     TTime duration,
                                     TTime timeWarp,
                                     TTime twEarly,
                                     TTime twLate,
                                     TTime release)
    : durationMatrix(durationMatrix),
      idxFirst(idxFirst),
      idxLast(idxLast),
      duration(duration),
      timeWarp(timeWarp),
      twEarly(twEarly),
      twLate(twLate),
      release(release)
{
}

#endif  // TIMEWINDOWDATA_H
