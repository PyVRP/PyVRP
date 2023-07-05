#ifndef PYVRP_TIMEWINDOWSEGMENT_H
#define PYVRP_TIMEWINDOWSEGMENT_H

#include "Matrix.h"
#include "Measure.h"

class TimeWindowSegment
{
    using TWS = TimeWindowSegment;

    int idxFirst = 0;          // Index of the first client in the segment
    int idxLast = 0;           // Index of the last client in the segment
    Duration duration = 0;     // Total duration, incl. waiting and servicing
    Duration timeWarp = 0;     // Cumulative time warp
    Duration twEarly = 0;      // Earliest visit moment of first client
    Duration twLate = 0;       // Latest visit moment of first client
    Duration releaseTime = 0;  // Earliest allowed moment to leave the depot

    [[nodiscard]] inline TWS merge(Matrix<Duration> const &durationMatrix,
                                   TWS const &other) const;

public:
    template <typename... Args>
    [[nodiscard]] inline static TWS
    merge(Matrix<Duration> const &durationMatrix,
          TWS const &first,
          TWS const &second,
          Args... args);

    /**
     * Total time warp, that is, the time warp along the the segment, and
     * potential time warp due to too late a release time.
     */
    [[nodiscard]] inline Duration totalTimeWarp() const;

    TimeWindowSegment() = default;  // TODO at least require client index

    inline TimeWindowSegment(int idxFirst,
                             int idxLast,
                             Duration duration,
                             Duration timeWarp,
                             Duration twEarly,
                             Duration twLate,
                             Duration releaseTime);
};

TimeWindowSegment TimeWindowSegment::merge(
    [[maybe_unused]] Matrix<Duration> const &durationMatrix,
    [[maybe_unused]] TimeWindowSegment const &other) const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return {};
#else
    auto const arcDuration = durationMatrix(idxLast, other.idxFirst);
    auto const diff = duration - timeWarp + arcDuration;
    auto const diffWait = std::max<Duration>(other.twEarly - diff - twLate, 0);
    auto const diffTw = std::max<Duration>(twEarly + diff - other.twLate, 0);

    return {idxFirst,
            other.idxLast,
            duration + other.duration + arcDuration + diffWait,
            timeWarp + other.timeWarp + diffTw,
            std::max(other.twEarly - diff, twEarly) - diffWait,
            std::min(other.twLate - diff, twLate) + diffTw,
            std::max(releaseTime, other.releaseTime)};
#endif
}

template <typename... Args>
TimeWindowSegment TimeWindowSegment::merge(
    [[maybe_unused]] Matrix<Duration> const &durationMatrix,
    [[maybe_unused]] TimeWindowSegment const &first,
    [[maybe_unused]] TimeWindowSegment const &second,
    [[maybe_unused]] Args... args)
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return {};
#else
    auto const res = first.merge(durationMatrix, second);

    if constexpr (sizeof...(args) == 0)
        return res;
    else
        return merge(durationMatrix, res, args...);
#endif
}

Duration TimeWindowSegment::totalTimeWarp() const
{
    return timeWarp + std::max<Duration>(releaseTime - twLate, 0);
}

TimeWindowSegment::TimeWindowSegment(int idxFirst,
                                     int idxLast,
                                     Duration duration,
                                     Duration timeWarp,
                                     Duration twEarly,
                                     Duration twLate,
                                     Duration releaseTime)
    : idxFirst(idxFirst),
      idxLast(idxLast),
      duration(duration),
      timeWarp(timeWarp),
      twEarly(twEarly),
      twLate(twLate),
      releaseTime(releaseTime)
{
}

#endif  // PYVRP_TIMEWINDOWSEGMENT_H
