#include "DurationSegment.h"

#include <fstream>

using pyvrp::Duration;
using pyvrp::DurationSegment;

DurationSegment DurationSegment::finaliseBack() const
{
    // We finalise this segment via several repeated merges: first, from the
    // [end early, end late] time windows of the previous trip. Then, the
    // release times of our current trip, if they are binding. Finally, we merge
    // with the current trip, using just the earliest and latest start moments
    // implied by our time windows. This results in a finalised segment.
    DurationSegment const prev = {0, 0, endEarly_, endLate_, 0};
    DurationSegment const curr = {duration_, timeWarp_, twEarly_, twLate_, 0};
    DurationSegment const release = {0,
                                     0,
                                     std::max(twEarly_, releaseTime_),
                                     std::max(twLate_, releaseTime_),
                                     0};

    auto const finalised = merge(0, merge(0, prev, release), curr);
    auto const netDur = finalised.tripDuration() - finalised.tripTimeWarp();
    auto const endLate
        = netDur > std::numeric_limits<Duration>::max() - finalised.twLate()
              ? std::numeric_limits<Duration>::max()
              : finalised.twLate() + netDur;

    return {0,
            0,
            finalised.twEarly() + netDur,
            // The next segment after this is free to start at any time after
            // this segment can end, so the latest start is not constrained.
            // Starting after our latest end will incur wait duration.
            std::numeric_limits<Duration>::max(),
            0,
            cumDuration_ + finalised.duration(),
            cumTimeWarp_ + finalised.timeWarp(),
            finalised.twEarly() + netDur,
            endLate};
}

DurationSegment DurationSegment::finaliseFront() const
{
    // We finalise at the start of this segment. This is pretty easy: we just
    // need to make sure our segment is visited between twEarly and twLate,
    // like before.
    return {0, 0, twEarly(), twLate(), 0, duration(), timeWarp()};
}

Duration DurationSegment::tripDuration() const
{
    return duration() - cumDuration_;
}

Duration DurationSegment::tripTimeWarp() const
{
    return timeWarp() - cumTimeWarp_;
}

Duration DurationSegment::twLate() const { return twLate_; }

Duration DurationSegment::endEarly() const { return endEarly_; }

Duration DurationSegment::endLate() const { return endLate_; }

Duration DurationSegment::slack() const
{
    return std::min(twLate() - twEarly(), endLate() - endEarly());
}

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
