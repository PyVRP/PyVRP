#include "DurationSegment.h"

using pyvrp::Duration;
using pyvrp::DurationSegment;

Duration DurationSegment::duration() const { return duration_; }

Duration DurationSegment::twEarly() const { return twEarly_; }

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
