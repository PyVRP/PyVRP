#ifndef PYVRP_DURATIONSEGMENT_H
#define PYVRP_DURATIONSEGMENT_H

#include "Matrix.h"
#include "Measure.h"
#include "ProblemData.h"

namespace pyvrp
{
/**
 * DurationSegment(
 *     idx_first: int,
 *     idx_last: int,
 *     duration: int,
 *     time_warp: int,
 *     tw_early: int,
 *     tw_late: int,
 *     release_time: int,
 * )
 *
 * Creates a duration segment.
 *
 * Duration segments can be efficiently concatenated, and track statistics
 * about route duration and time warp resulting from visiting clients in the
 * concatenated order.
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
class DurationSegment
{
    size_t idxFirst_;    // Index of the first client in the segment
    size_t idxLast_;     // Index of the last client in the segment
    Duration duration_;  // Total duration, incl. waiting and servicing
    // Duration timeWarp_;      // Cumulative time warp
    Duration twEarly_;  // Earliest visit moment of first client
    // Duration twLate_;        // Latest visit moment of first client
    Duration latestFinish_;  // Latest finish moment of last client
    Duration releaseTime_;   // Earliest allowed moment to leave the depot

    [[nodiscard]] inline DurationSegment
    merge(Matrix<Duration> const &durationMatrix,
          DurationSegment const &other) const;

public:
    template <typename... Args>
    [[nodiscard]] static DurationSegment
    merge(Matrix<Duration> const &durationMatrix,
          DurationSegment const &first,
          DurationSegment const &second,
          Args &&...args);

    /**
     * The total duration of this route segment.
     */
    [[nodiscard]] inline Duration duration() const;

    /**
     * Returns the time warp on this route segment. Additionally, any time warp
     * incurred by violating the maximum duration argument is also counted.
     *
     * Parameters
     * ----------
     * max_duration
     *     Maximum allowed duration, if provided. If the segment's duration
     *     exceeds this value, any excess duration is counted as time warp.
     *     Default unconstrained.
     *
     * Returns
     * -------
     * int
     *     Total time warp on this route segment.
     */
    [[nodiscard]] inline Duration
    timeWarp(Duration const maxDuration
             = std::numeric_limits<Duration>::max()) const;

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

    // Construct from attributes of the given client.
    DurationSegment(size_t idx, ProblemData::Client const &client);

    // Construct from raw data.
    inline DurationSegment(size_t idxFirst,
                           size_t idxLast,
                           Duration duration,
                           Duration timeWarp,
                           Duration twEarly,
                           Duration twLate,
                           Duration releaseTime);

    // Construct from raw data.
    inline DurationSegment(size_t idxFirst,
                           size_t idxLast,
                           Duration duration,
                           Duration twEarly,
                           Duration latestFinish,
                           Duration releaseTime);

    // Move or copy construct from the other duration segment.
    inline DurationSegment(DurationSegment const &) = default;
    inline DurationSegment(DurationSegment &&) = default;

    // Move or copy assign form the other duration segment.
    inline DurationSegment &operator=(DurationSegment const &) = default;
    inline DurationSegment &operator=(DurationSegment &&) = default;
};

DurationSegment DurationSegment::merge(Matrix<Duration> const &durationMatrix,
                                       DurationSegment const &other) const
{
    using Dur = pyvrp::Duration;

    // edgeDuration is the travel duration from our last to the other's first
    // client, and atOther the time (after starting from our first client) at
    // which we arrive there.
    Dur const timeWarp = std::max<Dur>(twEarly_ + duration_ - latestFinish_, 0);
    Dur const twLate = std::max<Dur>(latestFinish_ - duration_, twEarly_);
    Dur const edgeDuration = durationMatrix(idxLast_, other.idxFirst_);
    Dur const atOther = duration_ - timeWarp + edgeDuration;

    // Time warp increases if we arrive at the other's first client after its
    // time window closes, whereas wait duration increases if we arrive there
    // before opening.
    Dur const otherTwLate
        = std::max<Dur>(other.latestFinish_ - other.duration_, other.twEarly_);
    Dur const diffTw = std::max<Dur>(twEarly_ + atOther - otherTwLate, 0);
    Dur const diffWait = other.twEarly_ - atOther > twLate
                             ? other.twEarly_ - atOther - twLate
                             : 0;  // ternary rather than max avoids underflow

    Dur const otherTimeWarp = std::max<Dur>(
        other.twEarly_ + other.duration_ - other.latestFinish_, 0);
    Dur const newTimeWarp = timeWarp + otherTimeWarp + diffTw;
    Dur const newTwLate = std::min(otherTwLate - atOther, twLate) + diffTw;
    Dur const newDuration
        = duration_ + other.duration_ + edgeDuration + diffWait;
    Dur const latestFinish = newTwLate + newDuration - newTimeWarp;
    return {idxFirst_,
            other.idxLast_,
            newDuration,
            std::max(other.twEarly_ - atOther, twEarly_) - diffWait,
            latestFinish,
            std::max(releaseTime_, other.releaseTime_)};
    // return {idxFirst_,
    //         other.idxLast_,
    //         newDuration,
    //         newTimeWarp,
    //         std::max(other.twEarly_ - atOther, twEarly_) - diffWait,
    //         newTwLate,
    //         std::max(releaseTime_, other.releaseTime_)};
}

template <typename... Args>
DurationSegment
DurationSegment::merge([[maybe_unused]] Matrix<Duration> const &durationMatrix,
                       [[maybe_unused]] DurationSegment const &first,
                       [[maybe_unused]] DurationSegment const &second,
                       [[maybe_unused]] Args &&...args)
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return {0, 0, 0, 0, 0, 0, 0};
#else
    auto const res = first.merge(durationMatrix, second);

    if constexpr (sizeof...(args) == 0)
        return res;
    else
        return merge(durationMatrix, res, args...);
#endif
}

Duration DurationSegment::duration() const { return duration_; }

Duration DurationSegment::timeWarp(Duration const maxDuration) const
{
    // clang-format off
    // assert(timeWarp_ == std::max<Duration>(twEarly_ + duration_ - latestFinish_, 0));
    Duration const twLate = std::max<Duration>(latestFinish_ - duration_, twEarly_);
    return std::max<Duration>(twEarly_ + duration_ - latestFinish_, 0)
         + std::max<Duration>(releaseTime_ - twLate, 0)
         + std::max<Duration>(duration_ - maxDuration, 0);
    // clang-format on
}

DurationSegment::DurationSegment(size_t idxFirst,
                                 size_t idxLast,
                                 Duration duration,
                                 Duration timeWarp,
                                 Duration twEarly,
                                 Duration twLate,
                                 Duration releaseTime)
    : idxFirst_(idxFirst),
      idxLast_(idxLast),
      duration_(duration),
      //   timeWarp_(timeWarp),
      twEarly_(twEarly),
      //   twLate_(twLate),
      latestFinish_(twLate + duration - timeWarp),
      releaseTime_(releaseTime)
{
}

DurationSegment::DurationSegment(size_t idxFirst,
                                 size_t idxLast,
                                 Duration duration,
                                 Duration twEarly,
                                 Duration latestFinish,
                                 Duration releaseTime)
    : idxFirst_(idxFirst),
      idxLast_(idxLast),
      duration_(duration),
      //   timeWarp_(std::max<Duration>(twEarly + duration - latestFinish, 0)),
      twEarly_(twEarly),
      //   twLate_(std::max<Duration>(latestFinish - duration, twEarly)),
      latestFinish_(latestFinish),
      releaseTime_(releaseTime)
{
}
}  // namespace pyvrp

#endif  // PYVRP_DURATIONSEGMENT_H
