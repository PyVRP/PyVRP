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

    size_t idxFirst_ = 0;    // Index of the first client in the segment
    size_t idxLast_ = 0;     // Index of the last client in the segment
    Duration duration_ = 0;  // Duration = travel + wait + service time
    Duration timeWarp_ = 0;  // Cumulative time warp (end - start = dur - tw)
    Duration twEarly_ = 0;   // Earliest moment to start service of first client
    Duration twLate_ = 0;    // Latest moment to start service of first client
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
    [[nodiscard]] inline Duration duration() const;
    /**
     * The total time warp on this route segment.
     */
    [[nodiscard]] inline Duration totalTimeWarp() const;
    /**
     * Earliest start time for this route segment that results in minimum route
     * segment duration.
     */
    [[nodiscard]] inline Duration twEarly() const;
    /**
     * Latest start time for this route segment that results in minimum route
     * segment duration.
     */
    [[nodiscard]] inline Duration twLate() const;
    /**
     * Earliest possible release time for this segment (latest release time of)
     * clients in this segment).
     */
    [[nodiscard]] inline Duration releaseTime() const;

    TimeWindowSegment() = default;  // TODO at least require client index

    inline TimeWindowSegment(size_t idxFirst,
                             size_t idxLast,
                             Duration duration,
                             Duration timeWarp,
                             Duration twEarly,
                             Duration twLate,
                             Duration releaseTime);

    inline TimeWindowSegment(size_t idx, ProblemData::Client const &client);
};

TimeWindowSegment TimeWindowSegment::merge(
    [[maybe_unused]] Matrix<Duration> const &durationMatrix,
    [[maybe_unused]] TimeWindowSegment const &other) const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return {};
#else
    auto const travelDuration = durationMatrix(idxLast_, other.idxFirst_);
    auto const diff = duration_ - timeWarp_ + travelDuration;
    auto const diffWait
        = std::max<Duration>(other.twEarly_ - diff - twLate_, 0);
    auto const diffTw = std::max<Duration>(twEarly_ + diff - other.twLate_, 0);

    return {idxFirst_,
            other.idxLast_,
            duration_ + other.duration_ + travelDuration + diffWait,
            timeWarp_ + other.timeWarp_ + diffTw,
            std::max(other.twEarly_ - diff, twEarly_) - diffWait,
            std::min(other.twLate_ - diff, twLate_) + diffTw,
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

Duration TimeWindowSegment::duration() const { return duration_; }

Duration TimeWindowSegment::totalTimeWarp() const
{
    return timeWarp_ + std::max<Duration>(releaseTime_ - twLate_, 0);
}

Duration TimeWindowSegment::twEarly() const { return twEarly_; }

Duration TimeWindowSegment::twLate() const { return twLate_; }

Duration TimeWindowSegment::releaseTime() const { return releaseTime_; }

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

TimeWindowSegment::TimeWindowSegment(size_t idx,
                                     ProblemData::Client const &client)
    : idxFirst_(idx),
      idxLast_(idx),
      duration_(client.serviceDuration),
      timeWarp_(0),
      twEarly_(client.twEarly),
      twLate_(client.twLate),
      releaseTime_(client.releaseTime)
{
}
}  // namespace pyvrp

#endif  // PYVRP_TIMEWINDOWSEGMENT_H
