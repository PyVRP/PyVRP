#ifndef PYVRP_TIMEWINDOWSEGMENT_H
#define PYVRP_TIMEWINDOWSEGMENT_H

#include "Matrix.h"
#include "Measure.h"
#include "ProblemData.h"

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

    size_t idxFirst_ = 0;       // Index of the first client in the segment
    size_t idxLast_ = 0;        // Index of the last client in the segment
    Duration duration_ = 0;     // Total duration, incl. waiting and servicing
    Duration timeWarp_ = 0;     // Cumulative time warp
    Duration twEarly_ = 0;      // Earliest visit moment of first client
    Duration twLate_ = 0;       // Latest visit moment of first client
    Duration releaseTime_ = 0;  // Earliest allowed moment to leave the depot

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
     * The total duration of this route segment.
     */
    [[nodiscard]] Duration duration() const;

    /**
     * Returns the total time warp on this route segment.
     *
     * Returns
     * -------
     * int
     *     Total time warp on this route segment.
     */
    [[nodiscard]] inline Duration totalTimeWarp() const;

    /**
     * Earliest start time for this route segment that results in minimum route
     * segment duration.
     */
    [[nodiscard]] Duration twEarly() const;

    /**
     * Latest start time for this route segment that results in minimum route
     * segment duration.
     */
    [[nodiscard]] Duration twLate() const;

    /**
     * Earliest possible release time of the clients in this route segment.
     */
    [[nodiscard]] Duration releaseTime() const;

    // TODO remove this constructor.
    TimeWindowSegment() = default;

    // Construct from attributes of the given client.
    TimeWindowSegment(size_t idx, ProblemData::Client const &client);

    // Construct from raw data.
    inline TimeWindowSegment(size_t idxFirst,
                             size_t idxLast,
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
    using Dur = pyvrp::Duration;

    // edgeDuration is the travel duration from our last to the other's first
    // client, and atOther the time (after starting from our first client) at
    // which we arrive there.
    Dur const edgeDuration = durationMatrix(idxLast_, other.idxFirst_);
    Dur const atOther = duration_ - timeWarp_ + edgeDuration;

    // Wait duration increases if we arrive at the other's first client before
    // opening, whereas time warp increases if we arrive there after closing.
    Dur const diffWait = std::max<Dur>(other.twEarly_ - atOther - twLate_, 0);
    Dur const diffTw = std::max<Dur>(twEarly_ + atOther - other.twLate_, 0);

    return {idxFirst_,
            other.idxLast_,
            duration_ + other.duration_ + edgeDuration + diffWait,
            timeWarp_ + other.timeWarp_ + diffTw,
            std::max(other.twEarly_ - atOther, twEarly_) - diffWait,
            std::min(other.twLate_ - atOther, twLate_) + diffTw,
            std::max(releaseTime_, other.releaseTime_)};
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
    return timeWarp_ + std::max<Duration>(releaseTime_ - twLate_, 0);
}

TimeWindowSegment::TimeWindowSegment(size_t idxFirst,
                                     size_t idxLast,
                                     Duration duration,
                                     Duration timeWarp,
                                     Duration twEarly,
                                     Duration twLate,
                                     Duration releaseTime)
    : idxFirst_(idxFirst),
      idxLast_(idxLast),
      duration_(duration),
      timeWarp_(timeWarp),
      twEarly_(twEarly),
      twLate_(twLate),
      releaseTime_(releaseTime)
{
}
}  // namespace pyvrp

#endif  // PYVRP_TIMEWINDOWSEGMENT_H
