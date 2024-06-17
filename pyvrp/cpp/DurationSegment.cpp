#include "DurationSegment.h"

using pyvrp::Duration;
using pyvrp::DurationSegment;

Duration DurationSegment::earliestStart() const { return earliestStart_; }

Duration DurationSegment::releaseTime() const { return releaseTime_; }

DurationSegment::DurationSegment(size_t idx, ProblemData::Client const &client)
    : idxFirst_(idx),
      idxLast_(idx),
      duration_(client.serviceDuration),
      earliestStart_(client.twEarly),
      latestFinish_(client.twLate > std::numeric_limits<Duration>::max()
                                        - client.serviceDuration
                        ? std::numeric_limits<Duration>::max()
                        : client.twLate + client.serviceDuration),
      releaseTime_(client.releaseTime)
{
}

DurationSegment::DurationSegment(ProblemData::Depot const &depot,
                                 ProblemData::VehicleType const &vehicleType)
    : idxFirst_(vehicleType.depot),
      idxLast_(vehicleType.depot),
      duration_(0),
      earliestStart_(std::max(depot.twEarly, vehicleType.twEarly)),
      latestFinish_(std::min(depot.twLate, vehicleType.twLate)),
      releaseTime_(0)
{
    // The start and finish times are limited by both the depot open and
    // closing times, and the vehicle's start and end of shift, whichever is
    // tighter. Note that since the duration is 0, the latest finish is equal
    // to the latest start as given by twLate for the depot and vehicle type.
}
