#include "DurationSegment.h"

#include <fstream>

using pyvrp::Duration;
using pyvrp::DurationSegment;

DurationSegment DurationSegment::finalise(Duration startTime) const
{
    // See Cattaruzza et al. (2016) for details. This function adapts equations
    // (11) -- (14) of https://doi.org/10.1287/trsc.2015.0608.
    auto const extraWait = std::max<Duration>(twEarly() - startTime, 0);
    auto const duration = duration_ + extraWait;

    auto const start = std::max(startTime, releaseTime_);
    auto const timeWarp = timeWarp_ + std::max<Duration>(start - twLate_, 0);

    return {0,
            0,
            startTime + duration - timeWarp,
            std::numeric_limits<Duration>::max(),
            0,
            cumDuration_ + duration,
            cumTimeWarp_ + timeWarp};
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
