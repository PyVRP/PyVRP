#include "TimeWindowSegment.h"

using pyvrp::Duration;
using pyvrp::TimeWindowSegment;

Duration TimeWindowSegment::duration() const { return duration_; }

Duration TimeWindowSegment::twEarly() const { return twEarly_; }

Duration TimeWindowSegment::twLate() const { return twLate_; }

Duration TimeWindowSegment::releaseTime() const { return releaseTime_; }

TimeWindowSegment::TimeWindowSegment(size_t idx,
                                     ProblemData::Client const &client)
    : idxFirst_(idx),
      idxLast_(idx),
      duration_(client.serviceDuration),
      timeWarp_(0),
      twEarly_(client.twEarly),
      twLate_(client.twLate),
      releaseTime_(client.releaseTime)
{
}
