#include "TimeWindowSegment.h"

TimeWindowSegment TimeWindowSegment::merge(TimeWindowSegment const &other) const
{
    int const distance = (*dist)(idxLast, other.idxFirst);
    int const delta = duration - timeWarp + distance;
    int const deltaWaitTime = std::max(other.twEarly - delta - twLate, 0);
    int const deltaTimeWarp = std::max(twEarly + delta - other.twLate, 0);

    return {dist,
            idxFirst,
            other.idxLast,
            duration + other.duration + distance + deltaWaitTime,
            timeWarp + other.timeWarp + deltaTimeWarp,
            std::max(other.twEarly - delta, twEarly) - deltaWaitTime,
            std::min(other.twLate - delta, twLate) + deltaTimeWarp,
            std::max(release, other.release)};
}

int TimeWindowSegment::segmentTimeWarp() const { return timeWarp; }

int TimeWindowSegment::totalTimeWarp() const
{
    return segmentTimeWarp() + std::max(release - twLate, 0);
}

TimeWindowSegment::TimeWindowSegment(Matrix<int> const *dist,
                                     int idxFirst,
                                     int idxLast,
                                     int duration,
                                     int timeWarp,
                                     int twEarly,
                                     int twLate,
                                     int release)
    : dist(dist),
      idxFirst(idxFirst),
      idxLast(idxLast),
      duration(duration),
      timeWarp(timeWarp),
      twEarly(twEarly),
      twLate(twLate),
      release(release)
{
}
