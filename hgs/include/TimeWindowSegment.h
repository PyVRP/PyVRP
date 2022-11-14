#ifndef TIMEWINDOWDATA_H
#define TIMEWINDOWDATA_H

#include "Params.h"

class TimeWindowSegment
{
    using TWS = TimeWindowSegment;

    Params const *params = nullptr;

    int idxFirst = 0;     // Index of the first client in the segment
    int idxLast = 0;      // Index of the last client in the segment
    int duration = 0;     // Total duration, incl. waiting and servicing
    int timeWarp = 0;     // Cumulative time warp
    int twEarly = 0;      // Earliest visit moment of first client in segment
    int twLate = 0;       // Latest visit moment of last client in segment
    int lastRelease = 0;  // Latest release time; cannot leave depot before

    [[nodiscard]] TWS merge(TWS const &other) const
    {
        int const dist = params->dist(idxLast, other.idxFirst);
        int const delta = duration - timeWarp + dist;
        int const deltaWaitTime = std::max(other.twEarly - delta - twLate, 0);
        int const deltaTimeWarp = std::max(twEarly + delta - other.twLate, 0);

        return {params,
                idxFirst,
                other.idxLast,
                duration + other.duration + dist + deltaWaitTime,
                timeWarp + other.timeWarp + deltaTimeWarp,
                std::max(other.twEarly - delta, twEarly) - deltaWaitTime,
                std::min(other.twLate - delta, twLate) + deltaTimeWarp,
                std::max(lastRelease, other.lastRelease)};
    }

public:
    template <typename... Args>
    [[nodiscard]] static TWS
    merge(TWS const &first, TWS const &second, Args... args)
    {
        auto const res = first.merge(second);

        if constexpr (sizeof...(args) == 0)
            return res;
        else
            return merge(res, args...);
    }

    /**
     * Returns the time warp along the segment, assuming we can depart in time.
     */
    [[nodiscard]] int segmentTimeWarp() const { return timeWarp; }

    /**
     * Total time warp, that is, the time warp along the the segment, and
     * potential time warp due to too late a release time.
     */
    [[nodiscard]] int totalTimeWarp() const
    {
        return segmentTimeWarp() + std::max(lastRelease - twLate, 0);
    }

    TimeWindowSegment() = default;  // TODO get rid of this constructor

    TimeWindowSegment(Params const *params,
                      int idxFirst,
                      int idxLast,
                      int duration,
                      int timeWarp,
                      int twEarly,
                      int twLate,
                      int latestReleaseTime)
        : params(params),
          idxFirst(idxFirst),
          idxLast(idxLast),
          duration(duration),
          timeWarp(timeWarp),
          twEarly(twEarly),
          twLate(twLate),
          lastRelease(latestReleaseTime)
    {
    }
};

#endif  // TIMEWINDOWDATA_H
