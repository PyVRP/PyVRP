#include "DurationSegment.h"

#include <fstream>

using pyvrp::Duration;
using pyvrp::DurationSegment;

Duration DurationSegment::prevEndLate() const { return prevEndLate_; }

Duration DurationSegment::releaseTime() const { return releaseTime_; }

Duration DurationSegment::slack() const
{
    // We have wait duration if release time is after the end of the previous
    // trip. Starting any later only increases that wait duration, so there
    // is then definitely no slack.
    auto const prevSlack = std::max<Duration>(prevEndLate_ - releaseTime_, 0);
    return std::min(twLate() - twEarly(), prevSlack);
}

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
               << ", prev_end_late=" << segment.prevEndLate();
    // clang-format on
}
