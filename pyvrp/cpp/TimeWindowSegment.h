#ifndef PYVRP_TIMEWINDOWSEGMENT_H
#define PYVRP_TIMEWINDOWSEGMENT_H

#include "Matrix.h"
#include "Measure.h"

namespace pyvrp
{
/**
 * TimeWindowSegment(
 *     idx_first: int = 0,
 *     idx_last: int = 0,
 *     duration: int = 0,
 *     time_warp: int = 0,
 *     tw_early: int = 0,
 *     tw_late: int = 0,
 *     release_time: int = 0,
 * )
 *
 * Creates a time window segment.
 *
 * Parameters
 * ----------
 * idx_first
 *     Index of the first client in the route segment.
 * idx_last
 *     Index of the last client in the route segment.
 * duration
 *     Total duration, including waiting time.
 * time_warp
 *     Total time warp on the route segment.
 * tw_early
 *     Earliest visit moment of the first client.
 * tw_late
 *     Latest visit moment of the first client.
 * release_time
 *     Earliest moment to start the route segment.
 */
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
     * Returns the total time warp on this route segment.
     *
     * Returns
     * -------
     * int
     *     Total time warp on this route segment.
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
}  // namespace pyvrp

#endif  // PYVRP_TIMEWINDOWSEGMENT_H
