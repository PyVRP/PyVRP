#include "DurationSegment.h"

#include <fstream>

using pyvrp::Duration;
using pyvrp::DurationSegment;

DurationSegment DurationSegment::finaliseBack() const
{
    auto const tripDuration = duration() - cumDuration_;
    auto const tripTimeWarp = timeWarp() - cumTimeWarp_;
    auto const netDuration = tripDuration - tripTimeWarp;

    // We finalise at the end of this segment. That means that a subsequent trip
    // can start at the earliest at twEarly + netDuration. This segment does not
    // constrain latest starts for subsequent trips, but we do track that this
    // segment ends at the latest at twLate + netDuration.
    return {0,
            0,
            twEarly() + netDuration,
            // The next segment after this is free to start at any time after
            // this segment can end, so the latest start is not constrained.
            // Starting after our latest end, however, incurs wait duration.
            // That is tracked by the endLate attribute.
            std::numeric_limits<Duration>::max(),
            0,
            duration(),
            timeWarp(),
            // The latest time at which this segment can end. A subsequent
            // segment can start later than this (since that's unconstrained),
            // but doing so adds wait duration between this end time and the
            // subsequent segment's start time.
            twLate() + netDuration};
}

DurationSegment DurationSegment::finaliseFront() const
{
    // We finalise at the start of this segment. This is pretty easy: we just
    // need to make sure our segment is visited between twEarly and twLate,
    // like before.
    return {0, 0, twEarly(), twLate(), 0, duration(), timeWarp()};
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

Duration DurationSegment::endLate() const { return endLate_; }

Duration DurationSegment::releaseTime() const { return releaseTime_; }

DurationSegment::DurationSegment(ProblemData::Client const &client)
    : duration_(client.serviceDuration),
      twEarly_(client.twEarly),
      twLate_(client.twLate),
      releaseTime_(client.releaseTime)
{
}

DurationSegment::DurationSegment(ProblemData::Depot const &depot)
    : twEarly_(depot.twEarly), twLate_(depot.twLate)
{
}

DurationSegment::DurationSegment(ProblemData::VehicleType const &vehicleType,
                                 Duration const twLate)
    : twEarly_(vehicleType.twEarly), twLate_(twLate)
{
}

std::ostream &operator<<(std::ostream &out, DurationSegment const &segment)
{
    // clang-format off
    return out << "duration=" << segment.duration() 
               << ", time_warp=" << segment.timeWarp()
               << ", tw_early=" << segment.twEarly()
               << ", tw_late=" << segment.twLate()
               << ", release_time=" << segment.releaseTime()
               << ", end_late=" << segment.endLate();
    // clang-format on
}
