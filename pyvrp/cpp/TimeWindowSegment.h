#ifndef TIMEWINDOWDATA_H
#define TIMEWINDOWDATA_H

#include "Matrix.h"
#include "ProblemData.h"
#include "precision.h"

class TimeWindowSegment
{
    using TWS = TimeWindowSegment;

    int idxFirst = 0;            // Index of the first client in the segment
    int idxLast = 0;             // Index of the last client in the segment
    duration_type duration = 0;  // Total duration, incl. waiting and servicing
    duration_type timeWarp = 0;  // Cumulative time warp
    duration_type twEarly = 0;   // Earliest visit moment of first client
    duration_type twLate = 0;    // Latest visit moment of last client

    [[nodiscard]] inline TWS merge(Matrix<duration_type> const &durMat,
                                   TWS const &other) const;

public:
    template <typename... Args>
    [[nodiscard]] inline static TWS merge(Matrix<duration_type> const &durMat,
                                          TWS const &first,
                                          TWS const &second,
                                          Args... args);

    /**
     * Total time warp, that is, the time warp along the the segment, and
     * potential time warp due to too late a release time.
     */
    [[nodiscard]] inline duration_type totalTimeWarp() const;

    TimeWindowSegment() = default;

    inline TimeWindowSegment(int idxFirst,
                             int idxLast,
                             duration_type duration,
                             duration_type timeWarp,
                             duration_type twEarly,
                             duration_type twLate);

    inline TimeWindowSegment(int idx, ProblemData::Client client);
};

TimeWindowSegment TimeWindowSegment::merge(Matrix<duration_type> const &durMat,
                                           TimeWindowSegment const &other) const
{
#ifdef VRP_NO_TIME_WINDOWS
    return {};
#else
    auto const travelDur = durMat(idxLast, other.idxFirst);
    auto const delta = duration - timeWarp + travelDur;
    auto const deltaWaitTime = std::max(other.twEarly - delta - twLate,
                                        static_cast<duration_type>(0));
    auto const deltaTimeWarp = std::max(twEarly + delta - other.twLate,
                                        static_cast<duration_type>(0));

    return {idxFirst,
            other.idxLast,
            duration + other.duration + travelDur + deltaWaitTime,
            timeWarp + other.timeWarp + deltaTimeWarp,
            std::max(other.twEarly - delta, twEarly) - deltaWaitTime,
            std::min(other.twLate - delta, twLate) + deltaTimeWarp};
#endif
}

template <typename... Args>
TimeWindowSegment TimeWindowSegment::merge(Matrix<duration_type> const &durMat,
                                           TimeWindowSegment const &first,
                                           TimeWindowSegment const &second,
                                           Args... args)
{
#ifdef VRP_NO_TIME_WINDOWS
    return {};
#else
    auto const res = first.merge(durMat, second);

    if constexpr (sizeof...(args) == 0)
        return res;
    else
        return merge(durMat, res, args...);
#endif
}

duration_type TimeWindowSegment::totalTimeWarp() const { return timeWarp; }

TimeWindowSegment::TimeWindowSegment(int idxFirst,
                                     int idxLast,
                                     duration_type duration,
                                     duration_type timeWarp,
                                     duration_type twEarly,
                                     duration_type twLate)
    : idxFirst(idxFirst),
      idxLast(idxLast),
      duration(duration),
      timeWarp(timeWarp),
      twEarly(twEarly),
      twLate(twLate)
{
}

TimeWindowSegment::TimeWindowSegment(int idx, ProblemData::Client client)
    : idxFirst(idx),
      idxLast(idx),
      duration(client.serviceDuration),
      timeWarp(0),
      twEarly(client.twEarly),
      twLate(client.twLate)
{
}

#endif  // TIMEWINDOWDATA_H
