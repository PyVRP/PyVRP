#include "DurationSegment.h"

#include <fstream>

using pyvrp::Duration;
using pyvrp::DurationSegment;

DurationSegment DurationSegment::finalise() const
{
    auto const tripDuration = duration() - cumDuration_;
    auto const tripTimeWarp = timeWarp() - cumTimeWarp_;

    return {0,
            0,
            twEarly() + tripDuration - tripTimeWarp,
            std::numeric_limits<Duration>::max(),
            0,
            duration(),
            timeWarp()};
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

Duration DurationSegment::slack() const { return twLate() - twEarly(); }

Duration DurationSegment::releaseTime() const { return releaseTime_; }

DurationSegment::DurationSegment(ProblemData::Client const &client)
    : duration_(client.serviceDuration),
      timeWarp_(0),
      twEarly_(client.twEarly),
      twLate_(client.twLate),
      releaseTime_(client.releaseTime)
{
}

DurationSegment::DurationSegment(ProblemData::Depot const &depot,
                                 Duration const serviceDuration)
    : duration_(serviceDuration),
      timeWarp_(0),
      twEarly_(depot.twEarly),
      twLate_(depot.twLate),
      releaseTime_(0)
{
}

DurationSegment::DurationSegment(ProblemData::VehicleType const &vehicleType,
                                 Duration const twLate)
    : duration_(0),
      timeWarp_(0),
      twEarly_(vehicleType.twEarly),
      twLate_(twLate),
      releaseTime_(0)
{
}

std::ostream &operator<<(std::ostream &out, DurationSegment const &segment)
{
    // clang-format off
    return out << segment.twEarly() 
               << ' ' << segment.twLate()
               << ' ' << segment.releaseTime()
               << ' ' << segment.duration()
               << ' ' << segment.timeWarp();
    // clang-format on
}
