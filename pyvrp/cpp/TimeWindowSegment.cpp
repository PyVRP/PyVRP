#include "TimeWindowSegment.h"

using pyvrp::Duration;
using pyvrp::TimeWindowSegment;

TimeWindowSegment
TimeWindowSegment::merge(Matrix<Duration> const &durationMatrix,
                         TimeWindowSegment const &other) const
{
    using Dur = pyvrp::Duration;

    // edgeDuration is the travel duration from our last to the other's first
    // client, and atOther the time (after starting from our first client) at
    // which we arrive there.
    Dur const edgeDuration = durationMatrix(idxLast_, other.idxFirst_);
    Dur const atOther = duration_ - timeWarp_ + edgeDuration;

    // Time warp increases if we arrive at the other's first client after its
    // time window closes, whereas wait duration increases if we arrive there
    // before opening.
    Dur const diffTw = std::max<Dur>(twEarly_ + atOther - other.twLate_, 0);
    Dur const diffWait = other.twEarly_ - atOther > twLate_
                             ? other.twEarly_ - atOther - twLate_
                             : 0;  // ternary rather than max avoids underflow

    return {idxFirst_,
            other.idxLast_,
            duration_ + other.duration_ + edgeDuration + diffWait,
            timeWarp_ + other.timeWarp_ + diffTw,
            std::max(other.twEarly_ - atOther, twEarly_) - diffWait,
            std::min(other.twLate_ - atOther, twLate_) + diffTw,
            std::max(releaseTime_, other.releaseTime_)};
}

Duration TimeWindowSegment::duration() const { return duration_; }

Duration TimeWindowSegment::timeWarp(Duration const maxDuration) const
{
    // clang-format off
    return timeWarp_
         + std::max<Duration>(releaseTime_ - twLate_, 0)
         + std::max<Duration>(duration_ - maxDuration, 0);
    // clang-format on
}

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

TimeWindowSegment::TimeWindowSegment(size_t idxFirst,
                                     size_t idxLast,
                                     Duration duration,
                                     Duration timeWarp,
                                     Duration twEarly,
                                     Duration twLate,
                                     Duration releaseTime)
    : idxFirst_(idxFirst),
      idxLast_(idxLast),
      duration_(duration),
      timeWarp_(timeWarp),
      twEarly_(twEarly),
      twLate_(twLate),
      releaseTime_(releaseTime)
{
}
