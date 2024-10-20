#include "DurationSegment.h"

#include <cassert>

using pyvrp::Duration;
using pyvrp::DurationSegment;

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

Duration DurationSegment::releaseTime() const { return releaseTime_; }

DurationSegment::DurationSegment(size_t idx, ProblemData::Client const &client)
    : idxFirst_(idx),
      idxLast_(idx),
      duration_(client.serviceDuration),
      timeWarp_(0),
      twEarly_(client.twEarly),
      twLate_(client.twLate),
      releaseTime_(client.releaseTime)
{
}

DurationSegment::DurationSegment(size_t depot,
                                 ProblemData::VehicleType const &vehicleType)
    : idxFirst_(depot),
      idxLast_(depot),
      duration_(0),
      timeWarp_(0),
      twEarly_(vehicleType.twEarly),
      twLate_(vehicleType.twLate),
      releaseTime_(0)
{
    assert(depot == vehicleType.startDepot || depot == vehicleType.endDepot);
}
