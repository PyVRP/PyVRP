#ifndef PYVRP_DURATIONSEGMENT_H
#define PYVRP_DURATIONSEGMENT_H

#include "Matrix.h"
#include "Measure.h"
#include "ProblemData.h"

#include <cassert>
#include <iosfwd>

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
    Duration duration_ = 0;     // total duration of current tip
    Duration timeWarp_ = 0;     // time warp on current trip
    Duration twEarly_ = 0;      // earliest start moment of current trip
    Duration twLate_ = 0;       // latest start moment of current trip
    Duration releaseTime_ = 0;  // earliest departure moment on current trip
    Duration cumDuration_ = 0;  // cumulative duration of other trips in segment
    Duration cumTimeWarp_ = 0;  // cumulative tw of other trips in segment

public:
    [[nodiscard]] static inline DurationSegment
    merge(Duration const edgeDuration,
          DurationSegment const &first,
          DurationSegment const &second);

    /**
     * Finalises this segment, and returns a new segment where release times
     * have been reset, and all other statistics have been suitably adjusted.
     * This is useful for multi-trip because the finalised segment can be
     * concatenated with segments of subsequent trips.
     */
    DurationSegment finalise() const;

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
     * Slack on this route, that is, the amount of time between the earliest
     * and latest route start.
     */
    [[nodiscard]] Duration slack() const;

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
                           Duration releaseTime,
                           Duration cumDuration = 0,
                           Duration cumTimeWarp = 0);

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
            std::max(first.releaseTime_, second.releaseTime_),
            first.cumDuration_ + second.cumDuration_,
            first.cumTimeWarp_ + second.cumTimeWarp_};
#endif
}

Duration DurationSegment::duration() const
{
    // We may need to wait for the release of our clients, in which case there
    // is additional wait time. But we only account for that on later trips,
    // because we assume that the first trip can simply be postponed.
    auto const wait = std::max<Duration>(twEarly() - twEarly_, 0);
    return cumDuration_ + duration_ + (cumDuration_ > 0 ? wait : 0);
}

Duration DurationSegment::timeWarp(Duration const maxDuration) const
{
    auto const timeWarp = cumTimeWarp_ + timeWarp_;
    auto const netDuration = duration() - timeWarp;

    return timeWarp
           + std::max<Duration>(releaseTime_ - twLate_, 0)
           // Max duration constraint applies only to net route duration,
           // subtracting existing time warp. Use ternary to avoid underflow.
           + (netDuration > maxDuration ? netDuration - maxDuration : 0);
}

DurationSegment::DurationSegment(Duration duration,
                                 Duration timeWarp,
                                 Duration twEarly,
                                 Duration twLate,
                                 Duration releaseTime,
                                 Duration cumDuration,
                                 Duration cumTimeWarp)
    : duration_(duration),
      timeWarp_(timeWarp),
      twEarly_(twEarly),
      twLate_(twLate),
      releaseTime_(releaseTime),
      cumDuration_(cumDuration),
      cumTimeWarp_(cumTimeWarp)
{
}
}  // namespace pyvrp

std::ostream &operator<<(std::ostream &out,
                         pyvrp::DurationSegment const &segment);

#endif  // PYVRP_DURATIONSEGMENT_H
