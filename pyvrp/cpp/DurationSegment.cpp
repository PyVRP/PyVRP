#include "DurationSegment.h"

using pyvrp::Duration;
using pyvrp::DurationSegment;

Duration DurationSegment::twEarly() const { return twEarly_; }

Duration DurationSegment::twLate() const
{
    return std::max<Duration>(latestFinish_ - duration_, twEarly_);
}

Duration DurationSegment::releaseTime() const { return releaseTime_; }

DurationSegment::DurationSegment(size_t idx, ProblemData::Client const &client)
    : idxFirst_(idx),
      idxLast_(idx),
      duration_(client.serviceDuration),
      // timeWarp_(0),
      twEarly_(client.twEarly),
      // twLate_(client.twLate),
      latestFinish_(client.twLate + client.serviceDuration),
      releaseTime_(client.releaseTime)
{
}
