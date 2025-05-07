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
 *     cum_duration: int = 0,
 *     cum_time_warp: int = 0,
 *     prev_end_late: int = np.iinfo(np.int64).max,
 * )
 *
 * Creates a duration segment.
 *
 * Duration segments can be efficiently concatenated, and track statistics
 * about route and trip duration and time warp resulting from visiting clients
 * in the concatenated order.
 *
 * Parameters
 * ----------
 * duration
 *     Total duration, including waiting time, of the current trip.
 * time_warp
 *     Total time warp on the current trip.
 * tw_early
 *     Earliest visit moment of the first client of the current trip.
 * tw_late
 *     Latest visit moment of the first client of the current trip.
 * release_time
 *     Earliest moment to start this trip segment at the depot.
 * cum_duration
 *     Cumulative duration of other trips in segment.
 * cum_time_warp
 *     Cumulative time warp of other trips in segment.
 * prev_end_late
 *     Latest end time of the previous trip, if any. Default unconstrained.
 */
class DurationSegment
{
    Duration duration_ = 0;                                   // of current trip
    Duration timeWarp_ = 0;                                   // of current trip
    Duration twEarly_ = 0;                                    // of current trip
    Duration twLate_ = std::numeric_limits<Duration>::max();  // of current trip
    Duration releaseTime_ = 0;                                // of current trip
    Duration cumDuration_ = 0;  // cumulative, excl. current trip
    Duration cumTimeWarp_ = 0;  // cumulative, excl. current trip
    Duration prevEndLate_
        = std::numeric_limits<Duration>::max();  // of prev trip

public:
    [[nodiscard]] static inline DurationSegment
    merge(Duration const edgeDuration,
          DurationSegment const &first,
          DurationSegment const &second);

    /**
     * Finalises this segment towards the back (at the end of the segment),
     * and returns a new segment where release times have been reset, and all
     * other statistics have been suitably adjusted. This is useful with
     * multiple trips because the finalised segment can be concatenated with
     * segments of later trips.
     */
    [[nodiscard]] inline DurationSegment finaliseBack() const;

    /**
     * Finalises this segment towards the front (at the start of the segment),
     * and returns a new segment where release times have been reset, and all
     * other statistics have been suitably adjusted. This is useful with
     * multiple trips because the finalised segment can be concatenated with
     * segments of earlier trips.
     */
    [[nodiscard]] inline DurationSegment finaliseFront() const;

    /**
     * The total duration of the whole segment.
     */
    [[nodiscard]] inline Duration duration() const;

    /**
     * Returns the time warp on this whole segment. Additionally, any time warp
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
    timeWarp(Duration maxDuration = std::numeric_limits<Duration>::max()) const;

    /**
     * Earliest start time for the current trip.
     */
    [[nodiscard]] inline Duration twEarly() const;

    /**
     * Latest start time for the current trip.
     */
    [[nodiscard]] inline Duration twLate() const;

    /**
     * Earliest end time of the current trip.
     */
    [[nodiscard]] inline Duration endEarly() const;

    /**
     * Latest end time of the current trip.
     */
    [[nodiscard]] inline Duration endLate() const;

    /**
     * Latest end time of the previous trip.
     */
    [[nodiscard]] Duration prevEndLate() const;

    /**
     * Release time of the clients on the current trip of this segment.
     */
    [[nodiscard]] Duration releaseTime() const;

    DurationSegment() = default;  // default is all zero

    // Construct from attributes of the given client.
    DurationSegment(ProblemData::Client const &client);

    /**
     * Construct from attributes of the given depot.
     */
    DurationSegment(ProblemData::Depot const &depot);

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
                           Duration cumTimeWarp = 0,
                           Duration prevEndLate
                           = std::numeric_limits<Duration>::max());

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
            first.cumTimeWarp_ + second.cumTimeWarp_,
            first.prevEndLate_};  // field is evaluated left-to-right
#endif
}

DurationSegment DurationSegment::finaliseBack() const
{
    // We finalise this segment via several repeated merges: first, from the
    // [end early, end late] time windows of the previous trip. Then, the
    // release times of our current trip, if they are binding. Finally, we merge
    // with the current trip, using just the earliest and latest start moments
    // implied by our time windows. This results in a finalised segment.
    DurationSegment const prev = {0, 0, 0, prevEndLate_, 0};
    DurationSegment const curr = {duration_, timeWarp_, twEarly_, twLate_, 0};
    DurationSegment const release = {0,
                                     0,
                                     std::max(twEarly_, releaseTime_),
                                     std::max(twLate_, releaseTime_),
                                     0};

    auto const finalised = merge(0, merge(0, prev, release), curr);
    return {0,
            0,
            finalised.endEarly(),
            // The next segment after this is free to start at any time after
            // this segment can end, so the latest start is not constrained.
            // However, starting after our latest end will incur wait duration.
            std::numeric_limits<Duration>::max(),
            finalised.endEarly(),
            cumDuration_ + finalised.duration(),
            cumTimeWarp_ + finalised.timeWarp(),
            finalised.endLate()};
}

DurationSegment DurationSegment::finaliseFront() const
{
    // We finalise at the start of this segment. This is pretty easy, via a
    // merge on an artificial node with our release times, if they are binding.
    DurationSegment const curr = {duration_, timeWarp_, twEarly_, twLate_, 0};
    DurationSegment const release = {0,
                                     0,
                                     std::max(twEarly_, releaseTime_),
                                     std::max(twLate_, releaseTime_),
                                     0};

    return merge(0, release, curr);
}

Duration DurationSegment::duration() const
{
    auto const duration = cumDuration_ + duration_;
    return duration + std::max<Duration>(twEarly() - prevEndLate_, 0);
}

Duration DurationSegment::timeWarp(Duration maxDuration) const
{
    auto const timeWarp = cumTimeWarp_ + timeWarp_;
    auto const netDuration = duration() - timeWarp;

    return timeWarp
           + std::max<Duration>(releaseTime_ - twLate_, 0)
           // Max duration constraint applies only to net route duration,
           // subtracting existing time warp. Use ternary to avoid underflow.
           + (netDuration > maxDuration ? netDuration - maxDuration : 0);
}

Duration DurationSegment::twEarly() const
{
    // There are two cases:
    // 1) When twLate_ < releaseTime_ there is time warp from release times. As
    //    twEarly_ <= twLate, we then return twLate_ to minimise this time warp.
    // 2) When twLate >= releaseTime_, there is a feasible start time that does
    //    not cause time warp due to release times. Then we return either the
    //    earliest start time, or the release time, whichever is larger.
    assert(twEarly_ <= twLate_);
    return std::max(twEarly_, std::min(twLate_, releaseTime_));
}

Duration DurationSegment::twLate() const { return twLate_; }

Duration DurationSegment::endEarly() const
{
    auto const tripDuration = duration() - cumDuration_;
    auto const tripTimeWarp = timeWarp() - cumTimeWarp_;
    return twEarly() + tripDuration - tripTimeWarp;
}

Duration DurationSegment::endLate() const
{
    auto const tripDuration = duration() - cumDuration_;
    auto const tripTimeWarp = timeWarp() - cumTimeWarp_;
    auto const netDuration = tripDuration - tripTimeWarp;
    return netDuration > std::numeric_limits<Duration>::max() - twLate()
               ? std::numeric_limits<Duration>::max()
               : twLate() + netDuration;
}

DurationSegment::DurationSegment(Duration duration,
                                 Duration timeWarp,
                                 Duration twEarly,
                                 Duration twLate,
                                 Duration releaseTime,
                                 Duration cumDuration,
                                 Duration cumTimeWarp,
                                 Duration prevEndLate)
    : duration_(duration),
      timeWarp_(timeWarp),
      twEarly_(twEarly),
      twLate_(twLate),
      releaseTime_(releaseTime),
      cumDuration_(cumDuration),
      cumTimeWarp_(cumTimeWarp),
      prevEndLate_(prevEndLate)
{
}
}  // namespace pyvrp

std::ostream &operator<<(std::ostream &out,  // helpful for debugging
                         pyvrp::DurationSegment const &segment);

#endif  // PYVRP_DURATIONSEGMENT_H
