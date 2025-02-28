#ifndef PYVRP_DURATIONSEGMENT_H
#define PYVRP_DURATIONSEGMENT_H

#include "Matrix.h"
#include "Measure.h"
#include "ProblemData.h"

namespace pyvrp
{
/**
 * DurationSegment(
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
    Duration duration_ = 0;     // Total duration, incl. waiting and servicing
    Duration timeWarp_ = 0;     // Cumulative time warp
    Duration twEarly_ = 0;      // Earliest visit moment of first client
    Duration twLate_ = 0;       // Latest visit moment of first client
    Duration releaseTime_ = 0;  // Earliest allowed moment to leave the depot

public:
    [[nodiscard]] static inline DurationSegment
    merge(Duration const edgeDuration,
          DurationSegment const &first,
          DurationSegment const &second);

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

    DurationSegment() = default;  // default is all zero

    // Construct from attributes of the given client.
    DurationSegment(ProblemData::Client const &client);

    /**
     * Construct from attributes of the given depot and depot service duration.
     */
    DurationSegment(ProblemData::Depot const &depot,
                    Duration const serviceDuration);

    /**
     * Construct from attributes of the given vehicle type and latest finish.
     */
    DurationSegment(ProblemData::VehicleType const &vehicleType,
                    Duration const twLate);

    // Construct from raw data.
    inline DurationSegment(Duration duration,
                           Duration timeWarp,
                           Duration twEarly,
                           Duration twLate,
                           Duration releaseTime);

    // Move or copy construct from the other duration segment.
    inline DurationSegment(DurationSegment const &) = default;
    inline DurationSegment(DurationSegment &&) = default;

    // Move or copy assign form the other duration segment.
    inline DurationSegment &operator=(DurationSegment const &) = default;
    inline DurationSegment &operator=(DurationSegment &&) = default;
};

DurationSegment
DurationSegment::merge([[maybe_unused]] Duration const edgeDuration,
                       [[maybe_unused]] DurationSegment const &first,
                       [[maybe_unused]] DurationSegment const &second)
{
    // Because clients' default time windows are [0, INT_MAX], the ternaries in
    // this method are carefully designed to avoid integer over- and underflow
    // issues. Be very careful when changing things here!
#ifdef PYVRP_NO_TIME_WINDOWS
    return {};
#else
    // atSecond is the time (relative to our starting time) at which we arrive
    // at the second's initial location.
    auto const atSecond = first.duration_ - first.timeWarp_ + edgeDuration;

    // Time warp increases when we arrive after the time window closes.
    auto const diffTw = first.twEarly_ + atSecond > second.twLate_
                            ? first.twEarly_ + atSecond - second.twLate_
                            : 0;

    // Wait duration increases if we arrive before the time window opens.
    auto const diffWait = second.twEarly_ - atSecond > first.twLate_
                              ? second.twEarly_ - atSecond - first.twLate_
                              : 0;

    auto const secondLate  // new twLate for the second segment
        = atSecond > second.twLate_ - std::numeric_limits<Duration>::max()
              ? second.twLate_ - atSecond
              : second.twLate_;

    return {first.duration_ + second.duration_ + edgeDuration + diffWait,
            first.timeWarp_ + second.timeWarp_ + diffTw,
            std::max(second.twEarly_ - atSecond, first.twEarly_) - diffWait,
            std::min(secondLate, first.twLate_) + diffTw,
            std::max(first.releaseTime_, second.releaseTime_)};
#endif
}

Duration DurationSegment::duration() const { return duration_; }

Duration DurationSegment::timeWarp(Duration const maxDuration) const
{
    return timeWarp_
           + std::max<Duration>(releaseTime_ - twLate_, 0)
           // Max duration constraint applies only to net route duration,
           // subtracting existing time warp. Use ternary to avoid underflow.
           + (duration_ - timeWarp_ > maxDuration
                  ? duration_ - timeWarp_ - maxDuration
                  : 0);
}

DurationSegment::DurationSegment(Duration duration,
                                 Duration timeWarp,
                                 Duration twEarly,
                                 Duration twLate,
                                 Duration releaseTime)
    : duration_(duration),
      timeWarp_(timeWarp),
      twEarly_(twEarly),
      twLate_(twLate),
      releaseTime_(releaseTime)
{
}
}  // namespace pyvrp

#endif  // PYVRP_DURATIONSEGMENT_H
