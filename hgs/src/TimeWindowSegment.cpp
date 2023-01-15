#include "TimeWindowSegment.h"

[[nodiscard]] int TimeWindowSegment::segmentTimeWarp() const
{
    return timeWarp;
}

[[nodiscard]] int TimeWindowSegment::totalTimeWarp() const
{
    return segmentTimeWarp() + std::max(release - twLate, 0);
}
