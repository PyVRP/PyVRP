#include "TimeWindowSegment.h"

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
