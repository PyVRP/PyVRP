#include "TimeWindowSegment.h"

int TimeWindowSegment::segmentTimeWarp() const { return timeWarp; }

int TimeWindowSegment::totalTimeWarp() const
{
    return segmentTimeWarp() + std::max(release - twLate, 0);
}
