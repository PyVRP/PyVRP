#include "DurationSegment.h"

#include <fstream>

using pyvrp::Duration;
using pyvrp::DurationSegment;

DurationSegment DurationSegment::finaliseBack() const
{
    DurationSegment const prevTrip = {0, 0, endEarly_, endLate_, 0};
    DurationSegment const currTrip = {tripDuration(),
                                      tripTimeWarp(),
                                      twEarly(),
                                      twLate(),
                                      0,
                                      cumDuration_,
                                      cumTimeWarp_};

    auto const adjusted = merge(0, prevTrip, currTrip);
    auto const netDuration = adjusted.tripDuration() - adjusted.tripTimeWarp();

    return {0,
            0,
            adjusted.twEarly() + netDuration,
            // The next segment after this is free to start at any time after
            // this segment can end, so the latest start is not constrained.
            // Starting after our latest end will incur wait duration.
            std::numeric_limits<Duration>::max(),
            0,
            adjusted.duration(),
            adjusted.timeWarp(),
            adjusted.twEarly() + netDuration,
            adjusted.twLate() + netDuration};
}

DurationSegment DurationSegment::finaliseFront() const
{
    // We finalise at the start of this segment. This is pretty easy: we just
    // need to make sure our segment is visited between twEarly and twLate,
    // like before.
    return {0, 0, twEarly(), twLate(), 0, duration(), timeWarp()};
}

Duration DurationSegment::tripDuration() const { return duration_; }

Duration DurationSegment::tripTimeWarp() const { return timeWarp_; }

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

Duration DurationSegment::endEarly() const { return endEarly_; }

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
               << ", end_early=" << segment.endEarly()
               << ", end_late=" << segment.endLate();
    // clang-format on
}
